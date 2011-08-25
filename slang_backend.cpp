/*
 * Copyright 2010, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "slang_backend.h"

#include <string>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"

#include "clang/CodeGen/ModuleBuilder.h"

#include "clang/Frontend/CodeGenOptions.h"
#include "clang/Frontend/FrontendDiagnostic.h"

#include "llvm/Assembly/PrintModulePass.h"

#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/CodeGen/RegAllocRegistry.h"
#include "llvm/CodeGen/SchedulerRegistry.h"

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Metadata.h"

#include "llvm/Support/PassManagerBuilder.h"

#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetRegistry.h"

#include "llvm/MC/SubtargetFeature.h"

#include "slang_assert.h"
#include "BitWriter_2_9/ReaderWriter_2_9.h"

namespace slang {

void Backend::CreateFunctionPasses() {
  if (!mPerFunctionPasses) {
    mPerFunctionPasses = new llvm::FunctionPassManager(mpModule);
    mPerFunctionPasses->add(new llvm::TargetData(mpModule));

    llvm::PassManagerBuilder PMBuilder;
    PMBuilder.OptLevel = mCodeGenOpts.OptimizationLevel;
    PMBuilder.populateFunctionPassManager(*mPerFunctionPasses);
  }
  return;
}

void Backend::CreateModulePasses() {
  if (!mPerModulePasses) {
    mPerModulePasses = new llvm::PassManager();
    mPerModulePasses->add(new llvm::TargetData(mpModule));

    llvm::PassManagerBuilder PMBuilder;
    PMBuilder.OptLevel = mCodeGenOpts.OptimizationLevel;
    PMBuilder.SizeLevel = mCodeGenOpts.OptimizeSize;
    PMBuilder.SizeLevel = mCodeGenOpts.OptimizeSize;
    if (mCodeGenOpts.UnitAtATime) {
      PMBuilder.DisableUnitAtATime = 0;
    } else {
      PMBuilder.DisableUnitAtATime = 1;
    }

    if (mCodeGenOpts.UnrollLoops) {
      PMBuilder.DisableUnrollLoops = 0;
    } else {
      PMBuilder.DisableUnrollLoops = 1;
    }

    PMBuilder.DisableSimplifyLibCalls = false;
    PMBuilder.populateModulePassManager(*mPerModulePasses);
  }
  return;
}

bool Backend::CreateCodeGenPasses() {
  if ((mOT != Slang::OT_Assembly) && (mOT != Slang::OT_Object))
    return true;

  // Now we add passes for code emitting
  if (mCodeGenPasses) {
    return true;
  } else {
    mCodeGenPasses = new llvm::FunctionPassManager(mpModule);
    mCodeGenPasses->add(new llvm::TargetData(mpModule));
  }

  // Create the TargetMachine for generating code.
  std::string Triple = mpModule->getTargetTriple();

  std::string Error;
  const llvm::Target* TargetInfo =
      llvm::TargetRegistry::lookupTarget(Triple, Error);
  if (TargetInfo == NULL) {
    mDiags.Report(clang::diag::err_fe_unable_to_create_target) << Error;
    return false;
  }

  llvm::NoFramePointerElim = mCodeGenOpts.DisableFPElim;

  // Use hardware FPU.
  //
  // FIXME: Need to detect the CPU capability and decide whether to use softfp.
  // To use softfp, change following 2 lines to
  //
  //  llvm::FloatABIType = llvm::FloatABI::Soft;
  //  llvm::UseSoftFloat = true;
  llvm::FloatABIType = llvm::FloatABI::Hard;
  llvm::UseSoftFloat = false;

  // BCC needs all unknown symbols resolved at compilation time. So we don't
  // need any relocation model.
  llvm::Reloc::Model RM = llvm::Reloc::Static;


  // This is set for the linker (specify how large of the virtual addresses we
  // can access for all unknown symbols.)
  if (mpModule->getPointerSize() == llvm::Module::Pointer32)
    llvm::TargetMachine::setCodeModel(llvm::CodeModel::Small);
  else
    // The target may have pointer size greater than 32 (e.g. x86_64
    // architecture) may need large data address model
    llvm::TargetMachine::setCodeModel(llvm::CodeModel::Medium);

  // Setup feature string
  std::string FeaturesStr;
  if (mTargetOpts.CPU.size() || mTargetOpts.Features.size()) {
    llvm::SubtargetFeatures Features;

    for (std::vector<std::string>::const_iterator
             I = mTargetOpts.Features.begin(), E = mTargetOpts.Features.end();
         I != E;
         I++)
      Features.AddFeature(*I);

    FeaturesStr = Features.getString();
  }
  llvm::TargetMachine *TM =
      TargetInfo->createTargetMachine(Triple, mTargetOpts.CPU, FeaturesStr, RM);

  // Register scheduler
  llvm::RegisterScheduler::setDefault(llvm::createDefaultScheduler);

  // Register allocation policy:
  //  createFastRegisterAllocator: fast but bad quality
  //  createLinearScanRegisterAllocator: not so fast but good quality
  llvm::RegisterRegAlloc::setDefault((mCodeGenOpts.OptimizationLevel == 0) ?
                                     llvm::createFastRegisterAllocator :
                                     llvm::createLinearScanRegisterAllocator);

  llvm::CodeGenOpt::Level OptLevel = llvm::CodeGenOpt::Default;
  if (mCodeGenOpts.OptimizationLevel == 0) {
    OptLevel = llvm::CodeGenOpt::None;
  } else if (mCodeGenOpts.OptimizationLevel == 3) {
    OptLevel = llvm::CodeGenOpt::Aggressive;
  }

  llvm::TargetMachine::CodeGenFileType CGFT =
      llvm::TargetMachine::CGFT_AssemblyFile;
  if (mOT == Slang::OT_Object) {
    CGFT = llvm::TargetMachine::CGFT_ObjectFile;
  }
  if (TM->addPassesToEmitFile(*mCodeGenPasses, FormattedOutStream,
                              CGFT, OptLevel)) {
    mDiags.Report(clang::diag::err_fe_unable_to_interface_with_target);
    return false;
  }

  return true;
}

Backend::Backend(clang::Diagnostic *Diags,
                 const clang::CodeGenOptions &CodeGenOpts,
                 const clang::TargetOptions &TargetOpts,
                 PragmaList *Pragmas,
                 llvm::raw_ostream *OS,
                 Slang::OutputType OT)
    : ASTConsumer(),
      mCodeGenOpts(CodeGenOpts),
      mTargetOpts(TargetOpts),
      mpModule(NULL),
      mpOS(OS),
      mOT(OT),
      mGen(NULL),
      mPerFunctionPasses(NULL),
      mPerModulePasses(NULL),
      mCodeGenPasses(NULL),
      mLLVMContext(llvm::getGlobalContext()),
      mDiags(*Diags),
      mPragmas(Pragmas) {
  FormattedOutStream.setStream(*mpOS,
                               llvm::formatted_raw_ostream::PRESERVE_STREAM);
  mGen = CreateLLVMCodeGen(mDiags, "", mCodeGenOpts, mLLVMContext);
  return;
}

void Backend::Initialize(clang::ASTContext &Ctx) {
  mGen->Initialize(Ctx);

  mpModule = mGen->GetModule();

  return;
}

void Backend::HandleTopLevelDecl(clang::DeclGroupRef D) {
  mGen->HandleTopLevelDecl(D);
  return;
}

void Backend::HandleTranslationUnit(clang::ASTContext &Ctx) {
  HandleTranslationUnitPre(Ctx);

  mGen->HandleTranslationUnit(Ctx);

  // Here, we complete a translation unit (whole translation unit is now in LLVM
  // IR). Now, interact with LLVM backend to generate actual machine code (asm
  // or machine code, whatever.)

  // Silently ignore if we weren't initialized for some reason.
  if (!mpModule)
    return;

  llvm::Module *M = mGen->ReleaseModule();
  if (!M) {
    // The module has been released by IR gen on failures, do not double free.
    mpModule = NULL;
    return;
  }

  slangAssert(mpModule == M &&
              "Unexpected module change during LLVM IR generation");

  // Insert #pragma information into metadata section of module
  if (!mPragmas->empty()) {
    llvm::NamedMDNode *PragmaMetadata =
        mpModule->getOrInsertNamedMetadata(Slang::PragmaMetadataName);
    for (PragmaList::const_iterator I = mPragmas->begin(), E = mPragmas->end();
         I != E;
         I++) {
      llvm::SmallVector<llvm::Value*, 2> Pragma;
      // Name goes first
      Pragma.push_back(llvm::MDString::get(mLLVMContext, I->first));
      // And then value
      Pragma.push_back(llvm::MDString::get(mLLVMContext, I->second));

      // Create MDNode and insert into PragmaMetadata
      PragmaMetadata->addOperand(
          llvm::MDNode::get(mLLVMContext, Pragma));
    }
  }

  HandleTranslationUnitPost(mpModule);

  // Create passes for optimization and code emission

  // Create and run per-function passes
  CreateFunctionPasses();
  if (mPerFunctionPasses) {
    mPerFunctionPasses->doInitialization();

    for (llvm::Module::iterator I = mpModule->begin(), E = mpModule->end();
         I != E;
         I++)
      if (!I->isDeclaration())
        mPerFunctionPasses->run(*I);

    mPerFunctionPasses->doFinalization();
  }

  // Create and run module passes
  CreateModulePasses();
  if (mPerModulePasses)
    mPerModulePasses->run(*mpModule);

  switch (mOT) {
    case Slang::OT_Assembly:
    case Slang::OT_Object: {
      if (!CreateCodeGenPasses())
        return;

      mCodeGenPasses->doInitialization();

      for (llvm::Module::iterator I = mpModule->begin(), E = mpModule->end();
          I != E;
          I++)
        if (!I->isDeclaration())
          mCodeGenPasses->run(*I);

      mCodeGenPasses->doFinalization();
      break;
    }
    case Slang::OT_LLVMAssembly: {
      llvm::PassManager *LLEmitPM = new llvm::PassManager();
      LLEmitPM->add(llvm::createPrintModulePass(&FormattedOutStream));
      LLEmitPM->run(*mpModule);
      break;
    }
    case Slang::OT_Bitcode: {
      llvm::PassManager *BCEmitPM = new llvm::PassManager();
      if (getTargetAPI() < SLANG_ICS_TARGET_API) {
        // Pre-ICS targets must use the LLVM 2.9 BitcodeWriter
        BCEmitPM->add(llvm_2_9::createBitcodeWriterPass(FormattedOutStream));
      } else {
        BCEmitPM->add(llvm::createBitcodeWriterPass(FormattedOutStream));
      }
      BCEmitPM->run(*mpModule);
      break;
    }
    case Slang::OT_Nothing: {
      return;
    }
    default: {
      slangAssert(false && "Unknown output type");
    }
  }

  FormattedOutStream.flush();

  return;
}

void Backend::HandleTagDeclDefinition(clang::TagDecl *D) {
  mGen->HandleTagDeclDefinition(D);
  return;
}

void Backend::CompleteTentativeDefinition(clang::VarDecl *D) {
  mGen->CompleteTentativeDefinition(D);
  return;
}

Backend::~Backend() {
  delete mpModule;
  delete mGen;
  delete mPerFunctionPasses;
  delete mPerModulePasses;
  delete mCodeGenPasses;
  return;
}

}  // namespace slang
