#include "slang.hpp"
#include "slang_backend.hpp"

#include "llvm/Module.h"                /* for class llvm::Module */
#include "llvm/Metadata.h"              /* for class llvm::NamedMDNode */
#include "llvm/LLVMContext.h"           /* for llvm::getGlobalContext() */

#include "llvm/Target/TargetMachine.h"  /* for class llvm::TargetMachine and llvm::TargetMachine::AssemblyFile */
#include "llvm/Target/TargetOptions.h"  /* for
                                         *  variable bool llvm::UseSoftFloat
                                         *  FloatABI::ABIType llvm::FloatABIType
                                         *  bool llvm::NoZerosInBSS
                                         */
#include "llvm/Target/TargetRegistry.h"     /* for class llvm::TargetRegistry */
#include "llvm/Target/SubtargetFeature.h"   /* for class llvm::SubtargetFeature */

#include "llvm/CodeGen/RegAllocRegistry.h"      /* for class llvm::RegisterRegAlloc */
#include "llvm/CodeGen/SchedulerRegistry.h"     /* for class llvm::RegisterScheduler and llvm::createDefaultScheduler() */

#include "llvm/Assembly/PrintModulePass.h"      /* for function createPrintModulePass() */
#include "llvm/Bitcode/ReaderWriter.h"          /* for function createBitcodeWriterPass() */

#include "clang/AST/Decl.h"             /* for class clang::*Decl */
#include "clang/AST/DeclGroup.h"        /* for class clang::DeclGroupRef */
#include "clang/AST/ASTContext.h"       /* for class clang::ASTContext */

#include "clang/Basic/TargetInfo.h"     /* for class clang::TargetInfo */
#include "clang/Basic/Diagnostic.h"     /* for class clang::Diagnostic */
#include "clang/Basic/TargetOptions.h"  /* for class clang::TargetOptions */

#include "clang/Frontend/FrontendDiagnostic.h"      /* for clang::diag::* */

#include "clang/CodeGen/ModuleBuilder.h"    /* for class clang::CodeGenerator */
#include "clang/CodeGen/CodeGenOptions.h"   /* for class clang::CodeGenOptions */

namespace slang {

bool Backend::CreateCodeGenPasses() {
    if(mOutputType != SlangCompilerOutput_Assembly && mOutputType != SlangCompilerOutput_Obj)
        return true;

    /* Now we add passes for code emitting */
    if(mCodeGenPasses) {
        return true;
    } else {
        mCodeGenPasses = new llvm::FunctionPassManager(mpModule);
        mCodeGenPasses->add(new llvm::TargetData(*mpTargetData));
    }

    /* Create the TargetMachine for generating code. */
    std::string Triple = mpModule->getTargetTriple();

    std::string Error;
    const llvm::Target* TargetInfo = llvm::TargetRegistry::lookupTarget(Triple, Error);
    if(TargetInfo == NULL) {
        mDiags.Report(clang::diag::err_fe_unable_to_create_target) << Error;
        return false;
    }

    llvm::NoFramePointerElim = mCodeGenOpts.DisableFPElim;

    /*
     * Use hardware FPU.
     *
     * FIXME: Need to detect the CPU capability and decide whether to use softfp. To use softfp, change following 2 lines to
     *
     *  llvm::FloatABIType = llvm::FloatABI::Soft;
     *  llvm::UseSoftFloat = true;
     */
    llvm::FloatABIType = llvm::FloatABI::Hard;
    llvm::UseSoftFloat = false;

    llvm::TargetMachine::setRelocationModel(llvm::Reloc::Static);   /* ACC needs all unknown symbols resolved at compilation time.
                                                                        So we don't need any relocation model. */

    /* The target with pointer size greater than 32 (e.g. x86_64 architecture) may need large data address model */
    if(mpTargetData->getPointerSizeInBits() > 32)
        llvm::TargetMachine::setCodeModel(llvm::CodeModel::Medium);
    else
        llvm::TargetMachine::setCodeModel(llvm::CodeModel::Small);  /* This is set for the linker (specify how large of the virtual addresses
                                                                        we can access for all unknown symbols.) */

    /* setup feature string */
    std::string FeaturesStr;
    if(mTargetOpts.CPU.size() || mTargetOpts.Features.size()) {
        llvm::SubtargetFeatures Features;

        Features.setCPU(mTargetOpts.CPU);

        for(std::vector<std::string>::const_iterator it = mTargetOpts.Features.begin();
            it != mTargetOpts.Features.end();
            it++)
            Features.AddFeature(*it);

        FeaturesStr = Features.getString();
    }
    llvm::TargetMachine *TM = TargetInfo->createTargetMachine(Triple, FeaturesStr);

    /* Register scheduler */
    llvm::RegisterScheduler::setDefault(llvm::createDefaultScheduler);

    /* Register allocation policy:
     *  createLocalRegisterAllocator: fast but bad quality
     *  createLinearScanRegisterAllocator: not so fast but good quality
     */
    llvm::RegisterRegAlloc::setDefault((mCodeGenOpts.OptimizationLevel == 0) ? llvm::createLocalRegisterAllocator : llvm::createLinearScanRegisterAllocator);

    llvm::CodeGenOpt::Level OptLevel = llvm::CodeGenOpt::Default;
    if(mCodeGenOpts.OptimizationLevel == 0)
        OptLevel = llvm::CodeGenOpt::None;
    else if(mCodeGenOpts.OptimizationLevel == 3)
        OptLevel = llvm::CodeGenOpt::Aggressive;

    llvm::TargetMachine::CodeGenFileType CGFT = llvm::TargetMachine::CGFT_AssemblyFile;;
    if(mOutputType == SlangCompilerOutput_Obj)
        CGFT = llvm::TargetMachine::CGFT_ObjectFile;
    if(TM->addPassesToEmitFile(*mCodeGenPasses, FormattedOutStream, CGFT, OptLevel)) {
        mDiags.Report(clang::diag::err_fe_unable_to_interface_with_target);
        return false;
    }

    return true;
}

Backend::Backend(Diagnostic &Diags,
                 const CodeGenOptions& CodeGenOpts,
                 const TargetOptions& TargetOpts,
                 const PragmaList& Pragmas,
                 llvm::raw_ostream* OS,
                 SlangCompilerOutputTy OutputType,
                 SourceManager &SourceMgr,
                 bool AllowRSPrefix) :
    ASTConsumer(),
    mLLVMContext(llvm::getGlobalContext()),
    mDiags(Diags),
    mCodeGenOpts(CodeGenOpts),
    mTargetOpts(TargetOpts),
    mPragmas(Pragmas),
    mpOS(OS),
    mOutputType(OutputType),
    mpModule(NULL),
    mpTargetData(NULL),
    mGen(NULL),
    mPerFunctionPasses(NULL),
    mPerModulePasses(NULL),
    mCodeGenPasses(NULL),
    mSourceMgr(SourceMgr),
    mAllowRSPrefix(AllowRSPrefix)
{
    FormattedOutStream.setStream(*mpOS, llvm::formatted_raw_ostream::PRESERVE_STREAM);
    mGen = CreateLLVMCodeGen(mDiags, "", mCodeGenOpts, mLLVMContext);
    return;
}

void Backend::Initialize(ASTContext &Ctx) {
    mGen->Initialize(Ctx);

    mpModule = mGen->GetModule();
    mpTargetData = new llvm::TargetData(Slang::TargetDescription);

    return;
}

void Backend::HandleTopLevelDecl(DeclGroupRef D) {
    /* Disallow user-defined functions with prefix "rs" */
    if (!mAllowRSPrefix) {
        DeclGroupRef::iterator it;
        for (it = D.begin(); it != D.end(); it++) {
            FunctionDecl *FD = dyn_cast<FunctionDecl>(*it);
            if (!FD || !FD->isThisDeclarationADefinition()) continue;
            if (FD->getName().startswith("rs")) {
                mDiags.Report(FullSourceLoc(FD->getLocStart(), mSourceMgr),
                              mDiags.getCustomDiagID(Diagnostic::Error, "invalid function name prefix, \"rs\" is reserved: '%0'")) << FD->getNameAsString();
            }
        }
    }

    mGen->HandleTopLevelDecl(D);
    return;
}

void Backend::HandleTranslationUnit(ASTContext& Ctx) {
    mGen->HandleTranslationUnit(Ctx);

    /*
     * Here, we complete a translation unit (whole translation unit is now in LLVM IR).
     *  Now, interact with LLVM backend to generate actual machine code (asm or machine
     *  code, whatever.)
     */

    if(!mpModule || !mpTargetData)  /* Silently ignore if we weren't initialized for some reason. */
        return;

    llvm::Module* M = mGen->ReleaseModule();
    if(!M) {
        /* The module has been released by IR gen on failures, do not double free. */
        mpModule = NULL;
        return;
    }

    assert(mpModule == M && "Unexpected module change during LLVM IR generation");

    /* Insert #pragma information into metadata section of module */
    if(!mPragmas.empty()) {
        llvm::NamedMDNode* PragmaMetadata = llvm::NamedMDNode::Create(mLLVMContext, Slang::PragmaMetadataName, NULL, 0, mpModule);
        for(PragmaList::const_iterator it = mPragmas.begin();
            it != mPragmas.end();
            it++)
        {
            llvm::SmallVector<llvm::Value*, 2> Pragma;
            /* Name goes first */
            Pragma.push_back(llvm::MDString::get(mLLVMContext, it->first));
            /* And then value */
            Pragma.push_back(llvm::MDString::get(mLLVMContext, it->second));
            /* Create MDNode and insert into PragmaMetadata */
            PragmaMetadata->addOperand( llvm::MDNode::get(mLLVMContext, Pragma.data(), Pragma.size()) );
        }
    }

    HandleTranslationUnitEx(Ctx);

    /* Create passes for optimization and code emission */

    /* Create and run per-function passes */
    CreateFunctionPasses();
    if(mPerFunctionPasses) {
        mPerFunctionPasses->doInitialization();

        for(llvm::Module::iterator I = mpModule->begin();
            I != mpModule->end();
            I++)
            if(!I->isDeclaration())
                mPerFunctionPasses->run(*I);

        mPerFunctionPasses->doFinalization();
    }


    /* Create and run module passes */
    CreateModulePasses();
    if(mPerModulePasses)
        mPerModulePasses->run(*mpModule);

    switch(mOutputType) {
        case SlangCompilerOutput_Assembly:
        case SlangCompilerOutput_Obj:
            if(!CreateCodeGenPasses())
                return;

            mCodeGenPasses->doInitialization();

            for(llvm::Module::iterator I = mpModule->begin();
                I != mpModule->end();
                I++)
                if(!I->isDeclaration())
                    mCodeGenPasses->run(*I);

            mCodeGenPasses->doFinalization();
        break;

        case SlangCompilerOutput_LL:
        {
            llvm::PassManager* LLEmitPM = new llvm::PassManager();
            LLEmitPM->add(llvm::createPrintModulePass(&FormattedOutStream));
            LLEmitPM->run(*mpModule);
        }
        break;

        case SlangCompilerOutput_Bitcode:
        {
            llvm::PassManager* BCEmitPM = new llvm::PassManager();
            BCEmitPM->add(llvm::createBitcodeWriterPass(FormattedOutStream));
            BCEmitPM->run(*mpModule);
        }
        break;

        case SlangCompilerOutput_Nothing:
            return;
        break;

        default:
            assert(false && "Unknown output type");
        break;
    }

    FormattedOutStream.flush();

    return;
}

void Backend::HandleTagDeclDefinition(TagDecl* D) {
    mGen->HandleTagDeclDefinition(D);
    return;
}

void Backend::CompleteTentativeDefinition(VarDecl* D) {
    mGen->CompleteTentativeDefinition(D);
    return;
}

Backend::~Backend() {
    if(mpModule)
        delete mpModule;
    if(mpTargetData)
        delete mpTargetData;
    if(mGen)
        delete mGen;
    if(mPerFunctionPasses)
        delete mPerFunctionPasses;
    if(mPerModulePasses)
        delete mPerModulePasses;
    if(mCodeGenPasses)
        delete mCodeGenPasses;
    return;
}

}   /* namespace slang */
