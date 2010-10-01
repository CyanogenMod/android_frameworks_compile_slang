#ifndef _SLANG_COMILER_RS_PRAGMA_HANDLER_HPP
#   define _SLANG_COMILER_RS_PRAGMA_HANDLER_HPP

#include <string>

#include "clang/Lex/Pragma.h"

namespace clang {
class Token;
class IdentifierInfo;
class Preprocessor;
}

namespace slang {

class RSContext;

class RSPragmaHandler : public clang::PragmaHandler {
 protected:
  RSContext *mContext;

  RSPragmaHandler(llvm::StringRef Name, RSContext *Context)
      : clang::PragmaHandler(Name),
        mContext(Context) {
    return;
  }
  RSContext *getContext() const {
    return this->mContext;
  }

  virtual void handleItem(const std::string &Item) { return; }

  // Handle pragma like #pragma rs [name] ([item #1],[item #2],...,[item #i])
  void handleItemListPragma(clang::Preprocessor &PP,
                            clang::Token &FirstToken);

  // Handle pragma like #pragma rs [name]
  void handleNonParamPragma(clang::Preprocessor &PP,
                            clang::Token &FirstToken);

  // Handle pragma like #pragma rs [name] ("string literal")
  void handleOptionalStringLiateralParamPragma(clang::Preprocessor& PP,
                                               clang::Token& FirstToken);

 public:
  static RSPragmaHandler *CreatePragmaExportVarHandler(RSContext *Context);
  static RSPragmaHandler *CreatePragmaExportVarAllHandler(RSContext *Context);
  static RSPragmaHandler *CreatePragmaExportFuncHandler(RSContext *Context);
  static RSPragmaHandler *CreatePragmaExportFuncAllHandler(RSContext *Context);
  static RSPragmaHandler *CreatePragmaExportTypeHandler(RSContext *Context);
  static RSPragmaHandler *CreatePragmaJavaPackageNameHandler(RSContext *Context);
  static RSPragmaHandler *CreatePragmaReflectLicenseHandler(RSContext *Context);

  virtual void HandlePragma(clang::Preprocessor &PP,
                            clang::Token &FirstToken) = 0;
};

}   // namespace slang

#endif  // _SLANG_COMILER_RS_PRAGMA_HANDLER_HPP
