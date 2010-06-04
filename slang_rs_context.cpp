#include "slang.hpp"
#include "slang_rs_context.hpp"
#include "slang_rs_reflection.hpp"
#include "slang_rs_export_var.hpp"
#include "slang_rs_export_func.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_pragma_handler.hpp"

#include "clang/AST/Type.h"             /* for class clang::QualType */
#include "clang/AST/Decl.h"             /* for class clang::*Decl */
#include "clang/Index/Utils.h"          /* for class clang::idx::ResolveLocationInAST() */
#include "clang/AST/DeclBase.h"         /* for class clang::Decl and clang::DeclContext */
#include "clang/AST/ASTContext.h"       /* for class clang::ASTContext */
#include "clang/Basic/TargetInfo.h"     /* for class clang::TargetInfo */
#include "clang/Index/ASTLocation.h"    /* for class clang::idx::ASTLocation */

#include "llvm/LLVMContext.h"           /* for function llvm::getGlobalContext() */
#include "llvm/Target/TargetData.h"     /* for class llvm::TargetData */

using namespace clang::idx;

namespace slang {

RSContext::RSContext(Preprocessor* PP, const TargetInfo* Target) : 
    mPP(PP),
    mTarget(Target),
    mTargetData(NULL),
    mLLVMContext(llvm::getGlobalContext()),
    mRSExportVarPragma(NULL),
    mRSExportFuncPragma(NULL),
    mRSExportTypePragma(NULL)
{
    /* For #pragma rs export_var */
    mRSExportVarPragma = RSPragmaHandler::CreatePragmaExportVarHandler(this);
    if(mRSExportVarPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportVarPragma);

    /* For #pragma rs export_func */
    mRSExportFuncPragma = RSPragmaHandler::CreatePragmaExportFuncHandler(this);
    if(mRSExportFuncPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportFuncPragma);

    /* For #pragma rs export_type */
    mRSExportTypePragma = RSPragmaHandler::CreatePragmaExportTypeHandler(this);
    if(mRSExportTypePragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportTypePragma);

    mTargetData = new llvm::TargetData(Slang::TargetDescription);

    return;
}

bool RSContext::processExportVar(const VarDecl* VD) {
    assert(!VD->getName().empty() && "Variable name should not be empty");

    /* TODO: some check on variable */

    RSExportType* ET = RSExportType::CreateFromDecl(this, VD);
    if(!ET) 
        return false;

    RSExportVar* EV = new RSExportVar(this, VD, ET);
    if(EV == NULL)
        return false;
    else
        mExportVars.push_back(EV);

    return true;
}

bool RSContext::processExportFunc(const FunctionDecl* FD) {
    assert(!FD->getName().empty() && "Function name should not be empty");
    
    if(!FD->isThisDeclarationADefinition())
        return false;

    if(FD->getStorageClass() != FunctionDecl::None) {
        printf("RSContext::processExportFunc : cannot export extern or static function '%s'\n", FD->getName().str().c_str());
        return false;
    }

    RSExportFunc* EF = RSExportFunc::Create(this, FD);
    if(EF == NULL)
        return false;
    else
        mExportFuncs.push_back(EF);

    return true;
}


bool RSContext::processExportType(ASTContext& Ctx, const llvm::StringRef& Name) {
    TranslationUnitDecl* TUDecl = Ctx.getTranslationUnitDecl();

    assert(TUDecl != NULL && "Translation unit declaration (top-level declaration) is null object");

    const IdentifierInfo* II = mPP->getIdentifierInfo(Name);
    if(II == NULL)
        /* TODO: error: identifier @Name mark as an exportable type cannot be found */
        return false;

    DeclContext::lookup_const_result R = TUDecl->lookup(II);
    bool Done = false;
    RSExportType* ET = NULL;

    RSExportPointerType::IntegerType = Ctx.IntTy.getTypePtr();

    for(DeclContext::lookup_const_iterator I = R.first;
        I != R.second;
        I++) 
    {
        NamedDecl* const ND = *I;
        ASTLocation* LastLoc = new ASTLocation(ND);
        ASTLocation AL = ResolveLocationInAST(Ctx, ND->getLocStart(), LastLoc);

        delete LastLoc;
        if(AL.isDecl()) {
            Decl* D = AL.dyn_AsDecl();
            const Type* T = NULL;

            switch(D->getKind()) {
                case Decl::Typedef:
                    T = static_cast<const TypedefDecl*>(D)->getCanonicalDecl()->getUnderlyingType().getTypePtr();
                break;

                case Decl::Record:
                    T = static_cast<const RecordDecl*>(D)->getTypeForDecl();
                break;

                default:
                    /* unsupported, skip */
                break;
            }

            if(T != NULL)
                ET = RSExportType::Create(this, T);
        }
    }

    RSExportPointerType::IntegerType = NULL;

    return (ET != NULL);
}

void RSContext::processExport(ASTContext& Ctx) {
    /* Export variable */
    TranslationUnitDecl* TUDecl = Ctx.getTranslationUnitDecl();
    for(DeclContext::decl_iterator DI = TUDecl->decls_begin();
        DI != TUDecl->decls_end();
        DI++)
    {
        if(DI->getKind() == Decl::Var) {
            VarDecl* VD = (VarDecl*) (*DI);
            NeedExportVarSet::iterator EI = mNeedExportVars.find(VD->getName());
            if(EI != mNeedExportVars.end() && processExportVar(VD))
                mNeedExportVars.erase(EI);
        } else if(DI->getKind() == Decl::Function) {
            FunctionDecl* FD = (FunctionDecl*) (*DI);
            NeedExportFuncSet::iterator EI = mNeedExportFuncs.find(FD->getName());
            if(EI != mNeedExportFuncs.end() && processExportFunc(FD))
                mNeedExportFuncs.erase(EI);
        }
    }

    /* Finally, export type forcely set to be exported by user */
    for(NeedExportTypeSet::const_iterator EI = mNeedExportTypes.begin();
        EI != mNeedExportTypes.end();
        EI++)
        if(!processExportType(Ctx, EI->getKey()))
            printf("RSContext::processExport : failed to export type '%s'\n", EI->getKey().str().c_str());

    
    return;
}

bool RSContext::insertExportType(const llvm::StringRef& TypeName, RSExportType* ET) {
    ExportTypeMap::value_type* NewItem = ExportTypeMap::value_type::Create(TypeName.begin(), TypeName.end(), mExportTypes.getAllocator(), ET);

    if(mExportTypes.insert(NewItem)) {
        return true;
    } else {
        delete NewItem;
        return false;
    }
}

bool RSContext::reflectToJava(const char* OutputClassPath, const std::string& InputFileName, const std::string& OutputBCFileName) {
    RSReflection* R = new RSReflection(this);
    bool ret = R->reflect(OutputClassPath, InputFileName, OutputBCFileName); 
    if(!ret)
        printf("RSContext::reflectToJava : failed to do reflection (%s)\n", R->getLastError());
    delete R;
    return ret;
}

RSContext::~RSContext() {
    if(mRSExportVarPragma != NULL)
        delete mRSExportVarPragma;
    if(mRSExportFuncPragma != NULL)
        delete mRSExportFuncPragma;
    if(mRSExportTypePragma != NULL)
        delete mRSExportTypePragma;
    if(mTargetData != NULL)
        delete mTargetData;
}

}   /* namespace slang */
