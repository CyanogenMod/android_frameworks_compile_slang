#ifndef _SLANG_COMPILER_BACKEND_H
#define _SLANG_COMPILER_BACKEND_H

#include "llvm/PassManager.h"

#include "llvm/Target/TargetData.h"

#include "llvm/Support/StandardPasses.h"
#include "llvm/Support/FormattedStream.h"

#include "clang/AST/ASTConsumer.h"

#include "slang.h"
#include "slang_pragma_recorder.h"

namespace llvm {
  class formatted_raw_ostream;
  class LLVMContext;
  class NamedMDNode;
  class Module;
  class PassManager;
  class FunctionPassManager;
}

namespace clang {
  class CodeGenOptions;
  class CodeGenerator;
  class DeclGroupRef;
  class TagDecl;
  class VarDecl;
}

namespace slang {

class Backend : public clang::ASTConsumer {
 private:
  const clang::CodeGenOptions &mCodeGenOpts;
  const clang::TargetOptions &mTargetOpts;

  // Output stream
  llvm::raw_ostream *mpOS;
  Slang::OutputType mOT;

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

  void CreateFunctionPasses();
  void CreateModulePasses();
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
          Slang::OutputType OT);

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
