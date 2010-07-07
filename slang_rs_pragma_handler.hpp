#ifndef _SLANG_COMILER_RS_PRAGMA_HANDLER_HPP
#   define _SLANG_COMILER_RS_PRAGMA_HANDLER_HPP

#include <string>

#include "clang/Lex/Pragma.h"   /* for class PragmaHandler */

namespace clang {

class Token;
class IdentifierInfo;
class Preprocessor;

}   /* namespace clang */

namespace slang {

using namespace clang;

class RSContext;

class RSPragmaHandler : public PragmaHandler {
protected:
    RSContext* mContext;

    RSPragmaHandler(const IdentifierInfo* name, RSContext* Context) : PragmaHandler(name), mContext(Context) { return; }

    RSContext* getContext() const { return this->mContext; }

    virtual void handleItem(const std::string& Item) { return; }

    /* Handle pragma like #pragma rs [name] ([item #1],[item #2],...,[item #i]) */
    void handleItemListPragma(Preprocessor& PP, Token& FirstToken);

    /* Handle pragma like #pragma rs [name] */
    void handleNonParamPragma(Preprocessor& PP, Token& FirstToken);

    /* Handle pragma like #pragma rs [name] ("string literal") */
    void handleOptionalStringLiateralParamPragma(Preprocessor& PP, Token& FirstToken);
public:
    static RSPragmaHandler* CreatePragmaExportVarHandler(RSContext* Context);
    static RSPragmaHandler* CreatePragmaExportVarAllHandler(RSContext* Context);
    static RSPragmaHandler* CreatePragmaExportFuncHandler(RSContext* Context);
    static RSPragmaHandler* CreatePragmaExportFuncAllHandler(RSContext* Context);
    static RSPragmaHandler* CreatePragmaExportTypeHandler(RSContext* Context);
    static RSPragmaHandler* CreatePragmaJavaPackageNameHandler(RSContext* Context);
    static RSPragmaHandler* CreatePragmaReflectLicenseHandler(RSContext* Context);

    virtual void HandlePragma(Preprocessor& PP, Token& FirstToken) = 0;
};

}   /* namespace slang */

#endif  /* _SLANG_COMILER_RS_PRAGMA_HANDLER_HPP */
