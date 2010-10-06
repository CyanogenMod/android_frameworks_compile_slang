#ifndef _SLANG_COMPILER_RS_BACKEND_H
#define _SLANG_COMPILER_RENDER_SCRIPT_BACKEND_H

#include "slang_backend.h"
#include "slang_pragma_recorder.h"

namespace llvm {
  class NamedMDNode;
}

namespace clang {
  class ASTConsumer;
  class Diagnostic;
  class TargetOptions;
  class PragmaList;
  class CodeGenerator;
  class ASTContext;
  class DeclGroupRef;
}

namespace slang {

class RSContext;

class RSBackend : public Backend {
 private:
  RSContext *mContext;

  clang::SourceManager &mSourceMgr;

  bool mAllowRSPrefix;

  llvm::NamedMDNode *mExportVarMetadata;
  llvm::NamedMDNode *mExportFuncMetadata;
  llvm::NamedMDNode *mExportTypeMetadata;
  llvm::NamedMDNode *mExportElementMetadata;

  virtual void HandleTranslationUnitEx(clang::ASTContext &Ctx);

 public:
  RSBackend(RSContext *Context,
            clang::Diagnostic &Diags,
            const clang::CodeGenOptions &CodeGenOpts,
            const clang::TargetOptions &TargetOpts,
            const PragmaList &Pragmas,
            llvm::raw_ostream *OS,
            Slang::OutputType OT,
            clang::SourceManager &SourceMgr,
            bool AllowRSPrefix);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual ~RSBackend();
};
}

#endif  // _SLANG_COMPILER_BACKEND_H
