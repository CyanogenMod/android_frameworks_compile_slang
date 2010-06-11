#ifndef _SLANG_COMPILER_RS_BACKEND_HPP
#   define _SLANG_COMPILER_RENDER_SCRIPT_BACKEND_HPP

#include "libslang.h"
#include "slang_backend.hpp"
#include "slang_pragma_recorder.hpp"

#include "clang/CodeGen/CodeGenOptions.h"   /* for class clang::CodeGenOptions */

namespace llvm {

class NamedMDNode;

}   /* namespace llvm */

namespace clang {

class ASTConsumer;
class Diagnostic;
class TargetOptions;
class PragmaList;
class CodeGenerator;
class ASTContext;
class DeclGroupRef;

}   /* namespace clang */

namespace slang {

using namespace clang;

class RSContext;

class RSBackend : public Backend {
private:
    RSContext* mContext;

    llvm::NamedMDNode* mExportVarMetadata;
    llvm::NamedMDNode* mExportFuncMetadata;
    llvm::NamedMDNode* mExportTypeMetadata;
    llvm::NamedMDNode* mExportElementMetadata;

    virtual void HandleTranslationUnitEx(ASTContext& Ctx);

public:
    RSBackend(RSContext* Context,
              Diagnostic &Diags,
              const CodeGenOptions& CodeGenOpts,
              const TargetOptions& TargetOpts,
              const PragmaList& Pragmas,
              llvm::raw_ostream* OS,
              SlangCompilerOutputTy OutputType,
              SourceManager& SourceMgr);

    virtual void HandleTopLevelDecl(DeclGroupRef D);

    virtual ~RSBackend();
};  /* class RSBackend */

}   /* namespace slang */

#endif  /* _SLANG_COMPILER_BACKEND_HPP */
