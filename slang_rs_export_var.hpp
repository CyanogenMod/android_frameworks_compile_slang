#ifndef _SLANG_COMPILER_RS_EXPORT_VAR_HPP
#   define _SLANG_COMPILER_RS_EXPORT_VAR_HPP

#include "llvm/ADT/StringRef.h"     /* for class llvm::StringRef */

#include "clang/AST/Decl.h"         /* for clang::VarDecl */
#include "clang/AST/Expr.h"         /* for clang::Expr */

#include <string>

namespace clang {
    class APValue;
}

namespace slang {

using namespace clang;

class RSContext;
class RSExportType;

class RSExportVar {
    friend class RSContext;
private:
    RSContext* mContext;
    std::string mName;
    const RSExportType* mET;
    bool mIsConst;

    Expr::EvalResult mInit;

    RSExportVar(RSContext* Context, const VarDecl* VD, const RSExportType* ET);

public:
    inline const std::string& getName() const { return mName; }
    inline const RSExportType* getType() const { return mET; }
    inline RSContext* getRSContext() const { return mContext; }
    inline bool isConst() const { return mIsConst; }

    inline const APValue& getInit() const { return mInit.Val; }

};  /* RSExportVar */


}   /* namespace slang */

#endif  /* _SLANG_COMPILER_RS_EXPORT_VAR_HPP */
