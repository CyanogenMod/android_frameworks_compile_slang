#ifndef _SLANG_COMPILER_RS_BACKEND_HPP
#   define _SLANG_COMPILER_RENDER_SCRIPT_BACKEND_HPP

#include "libslang.h"
#include "slang_backend.hpp"
#include "slang_pragma_recorder.hpp"

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
            SlangCompilerOutputTy OutputType,
            clang::SourceManager &SourceMgr,
            bool AllowRSPrefix);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual ~RSBackend();
};

}

#endif  // _SLANG_COMPILER_BACKEND_HPP
