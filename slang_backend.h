#ifndef _SLANG_COMPILER_BACKEND_H
#define _SLANG_COMPILER_BACKEND_H

#include "llvm/PassManager.h"

#include "llvm/Target/TargetData.h"

#include "llvm/Support/StandardPasses.h"
#include "llvm/Support/FormattedStream.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CodeGenOptions.h"
#include "clang/Basic/SourceManager.h"

#include "libslang.h"
#include "slang_pragma_recorder.h"

namespace llvm {
  class LLVMContext;
  class NamedMDNode;
  class raw_ostream;
  class Module;
}

namespace clang {
  class ASTConsumer;
  class Diagnostic;
  class TargetOptions;
  class PragmaList;
  class CodeGenerator;
  class ASTContext;
  class DeclGroupRef;
  class TagDecl;
  class VarDecl;
}

namespace slang {

class Backend : public clang::ASTConsumer {
 private:
  const clang::CodeGenOptions &mCodeGenOpts;
  const clang::TargetOptions &mTargetOpts;

  clang::SourceManager &mSourceMgr;

  // Output stream
  llvm::raw_ostream *mpOS;
  SlangCompilerOutputTy mOutputType;

  llvm::TargetData *mpTargetData;

  // This helps us translate Clang AST using into LLVM IR
  clang::CodeGenerator *mGen;

  // Passes

  // Passes apply on function scope in a translation unit
  llvm::FunctionPassManager *mPerFunctionPasses;
  // Passes apply on module scope
  llvm::PassManager *mPerModulePasses;
  // Passes for code emission
  llvm::FunctionPassManager *mCodeGenPasses;

  llvm::formatted_raw_ostream FormattedOutStream;

  bool mAllowRSPrefix;

  inline void CreateFunctionPasses() {
    if (!mPerFunctionPasses) {
      mPerFunctionPasses = new llvm::FunctionPassManager(mpModule);
      mPerFunctionPasses->add(new llvm::TargetData(*mpTargetData));

      llvm::createStandardFunctionPasses(mPerFunctionPasses,
                                         mCodeGenOpts.OptimizationLevel);
    }
    return;
  }

  inline void CreateModulePasses() {
    if (!mPerModulePasses) {
      mPerModulePasses = new llvm::PassManager();
      mPerModulePasses->add(new llvm::TargetData(*mpTargetData));

      llvm::createStandardModulePasses(mPerModulePasses,
                                       mCodeGenOpts.OptimizationLevel,
                                       mCodeGenOpts.OptimizeSize,
                                       mCodeGenOpts.UnitAtATime,
                                       mCodeGenOpts.UnrollLoops,
                                       // Some libc functions will be replaced
                                       // by the LLVM built-in optimized
                                       // function (e.g. strcmp)
                                       /* SimplifyLibCalls */true,
                                       /* HaveExceptions */false,
                                       /* InliningPass */NULL);
    }

    // llvm::createStandardFunctionPasses and llvm::createStandardModulePasses
    // insert lots of optimization passes for the code generator. For the
    // conventional desktop PC which memory resources and computation power is
    // relatively large, doing lots optimization as possible is reasonible and
    // feasible. However, on the mobile device or embedded system, this may
    // cause some problem due to the hardware resources limitation. So they need
    // to be further refined.
    return;
  }

  bool CreateCodeGenPasses();

 protected:
  llvm::LLVMContext &mLLVMContext;
  clang::Diagnostic &mDiags;

  llvm::Module *mpModule;

  const PragmaList &mPragmas;

  // Extra handler for subclass to handle translation unit before emission
  virtual void HandleTranslationUnitEx(clang::ASTContext &Ctx) { return; }

 public:
  Backend(clang::Diagnostic &Diags,
          const clang::CodeGenOptions &CodeGenOpts,
          const clang::TargetOptions &TargetOpts,
          const PragmaList &Pragmas,
          llvm::raw_ostream *OS,
          SlangCompilerOutputTy OutputType,
          clang::SourceManager &SourceMgr,
          bool AllowRSPrefix);

  // Initialize - This is called to initialize the consumer, providing the
  // ASTContext.
  virtual void Initialize(clang::ASTContext &Ctx);

  // HandleTopLevelDecl - Handle the specified top-level declaration.  This is
  // called by the parser to process every top-level Decl*. Note that D can be
  // the head of a chain of Decls (e.g. for `int a, b` the chain will have two
  // elements). Use Decl::getNextDeclarator() to walk the chain.
  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  // HandleTranslationUnit - This method is called when the ASTs for entire
  // translation unit have been parsed.
  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  // HandleTagDeclDefinition - This callback is invoked each time a TagDecl
  // (e.g. struct, union, enum, class) is completed.  This allows the client to
  // hack on the type, which can occur at any point in the file (because these
  // can be defined in declspecs).
  virtual void HandleTagDeclDefinition(clang::TagDecl *D);

  // CompleteTentativeDefinition - Callback invoked at the end of a translation
  // unit to notify the consumer that the given tentative definition should be
  // completed.
  virtual void CompleteTentativeDefinition(clang::VarDecl *D);

  virtual ~Backend();
};

}   // namespace slang

#endif  // _SLANG_COMPILER_BACKEND_H
