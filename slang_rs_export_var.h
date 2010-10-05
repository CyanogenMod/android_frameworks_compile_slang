#ifndef _SLANG_COMPILER_RS_EXPORT_VAR_H
#define _SLANG_COMPILER_RS_EXPORT_VAR_H

#include "llvm/ADT/StringRef.h"

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include <string>

namespace clang {
  class APValue;
}

namespace slang {

  class RSContext;
  class RSExportType;

class RSExportVar {
  friend class RSContext;
 private:
  RSContext *mContext;
  std::string mName;
  const RSExportType *mET;
  bool mIsConst;

  clang::Expr::EvalResult mInit;

  RSExportVar(RSContext *Context,
              const clang::VarDecl *VD,
              const RSExportType *ET);

 public:
  inline const std::string &getName() const { return mName; }
  inline const RSExportType *getType() const { return mET; }
  inline RSContext *getRSContext() const { return mContext; }
  inline bool isConst() const { return mIsConst; }

  inline const clang::APValue &getInit() const { return mInit.Val; }
};  // RSExportVar

}   // namespace slang

#endif  // _SLANG_COMPILER_RS_EXPORT_VAR_H
