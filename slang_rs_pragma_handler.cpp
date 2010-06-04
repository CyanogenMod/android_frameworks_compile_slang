#include "slang_rs_context.hpp"
#include "slang_rs_pragma_handler.hpp"

#include "clang/Lex/Preprocessor.h"         /* for class Preprocessor */
#include "clang/Lex/Token.h"                /* for class Token */
#include "clang/Basic/TokenKinds.h"                /* for class Token */

#include "clang/Basic/IdentifierTable.h"    /* for class IdentifierInfo */

namespace {

using namespace clang;
using namespace slang;

class RSExportVarPragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->addExportVar(Item);
    }

public:
    RSExportVarPragmaHandler(IdentifierInfo* II, RSContext* Context) : RSPragmaHandler(II, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleItemListPragma(PP, FirstToken);
    }
};

class RSExportFuncPragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->addExportFunc(Item);
    }

public:
    RSExportFuncPragmaHandler(IdentifierInfo* II, RSContext* Context) : RSPragmaHandler(II, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleItemListPragma(PP, FirstToken);
    }
};

class RSExportTypePragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->addExportType(Item);
    }

public:
    RSExportTypePragmaHandler(IdentifierInfo* II, RSContext* Context) : RSPragmaHandler(II, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleItemListPragma(PP, FirstToken);
    }
};

}   /* anonymous namespace */

namespace slang {

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportVarHandler(RSContext* Context) {
    IdentifierInfo* II = Context->getPreprocessor()->getIdentifierInfo("export_var");
    if(II != NULL)
        return new RSExportVarPragmaHandler(II, Context);
    else
        return NULL;
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportFuncHandler(RSContext* Context) {
    IdentifierInfo* II = Context->getPreprocessor()->getIdentifierInfo("export_func");
    if(II != NULL)
        return new RSExportFuncPragmaHandler(II, Context);
    else
        return NULL;
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportTypeHandler(RSContext* Context) {
    IdentifierInfo* II = Context->getPreprocessor()->getIdentifierInfo("export_type");
    if(II != NULL)
        return new RSExportTypePragmaHandler(II, Context);
    else
        return NULL;
}

void RSPragmaHandler::handleItemListPragma(Preprocessor& PP, Token& FirstToken) {
    Token& PragmaToken = FirstToken;

    /* Skip first token, like "export_var" */
    PP.LexUnexpandedToken(PragmaToken);

    /* Now, the current token must be tok::lpara */
    if(PragmaToken.isNot(tok::l_paren))
        return;

    while(PragmaToken.isNot(tok::eom)) {
        /* Lex variable name */
        PP.LexUnexpandedToken(PragmaToken);
        if(PragmaToken.is(tok::identifier))
            this->handleItem( PP.getSpelling(PragmaToken) );
        else
            break;

        assert(PragmaToken.isNot(tok::eom));

        PP.LexUnexpandedToken(PragmaToken);

        if(PragmaToken.isNot(tok::comma))
            break;
    }
    return;
}

}   /* namespace slang */

