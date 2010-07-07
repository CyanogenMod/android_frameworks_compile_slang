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

class RSExportVarAllPragmaHandler : public RSPragmaHandler {
public:
    RSExportVarAllPragmaHandler(IdentifierInfo* II, RSContext* Context) : RSPragmaHandler(II, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleNonParamPragma(PP, FirstToken);
        mContext->setExportAllStaticVars(true);
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

class RSExportFuncAllPragmaHandler : public RSPragmaHandler {
public:
    RSExportFuncAllPragmaHandler(IdentifierInfo* II, RSContext* Context) : RSPragmaHandler(II, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleNonParamPragma(PP, FirstToken);
        mContext->setExportAllStaticFuncs(true);
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

class RSJavaPackageNamePragmaHandler : public RSPragmaHandler {
public:
    RSJavaPackageNamePragmaHandler(IdentifierInfo* II, RSContext* Context) : RSPragmaHandler(II, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        /* FIXME: Need to validate the extracted package name from paragma. Currently "all chars" specified in pragma will be treated as package name.
         *
         * 18.1 The Grammar of the Java Programming Language, http://java.sun.com/docs/books/jls/third_edition/html/syntax.html#18.1
         *
         * CompilationUnit:
         *         [[Annotations] package QualifiedIdentifier   ;  ] {ImportDeclaration}
         *         {TypeDeclaration}
         *
         * QualifiedIdentifier:
         *         Identifier { . Identifier }
         *
         * Identifier:
         *         IDENTIFIER
         *
         * 3.8 Identifiers, http://java.sun.com/docs/books/jls/third_edition/html/lexical.html#3.8
         *
         */

        Token& PragmaToken = FirstToken;
        std::string PackageName;

        /* Skip first token, "java_package_name" */
        PP.LexUnexpandedToken(PragmaToken);

        /* Now, the current token must be tok::lpara */
        if(PragmaToken.isNot(tok::l_paren))
            return;

        while(PragmaToken.isNot(tok::eom)) {
            /* Lex package name */
            PP.LexUnexpandedToken(PragmaToken);

            bool Invalid;
            std::string Spelling = PP.getSpelling(PragmaToken, &Invalid);
            if(!Invalid)
                PackageName.append(Spelling);

            /* Pre-mature end (syntax error will be triggered by preprocessor later) */
            if(PragmaToken.is(tok::eom) || PragmaToken.is(tok::eof))
                break;
            else {
                /* Next token is ')' (end of paragma) */
                const Token& NextTok = PP.LookAhead(0);
                if(NextTok.is(tok::r_paren)) {
                    mContext->setReflectJavaPackageName(PackageName);
                    /* Lex until meets tok::eom */
                    do
                        PP.LexUnexpandedToken(PragmaToken);
                    while(PragmaToken.isNot(tok::eom));
                    break;
                }
            }
        }
        return;
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

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportVarAllHandler(RSContext* Context) {
    IdentifierInfo* II = Context->getPreprocessor()->getIdentifierInfo("export_var_all");
    if(II != NULL)
        return new RSExportVarAllPragmaHandler(II, Context);
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

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportFuncAllHandler(RSContext* Context) {
    IdentifierInfo* II = Context->getPreprocessor()->getIdentifierInfo("export_func_all");
    if(II != NULL)
        return new RSExportFuncAllPragmaHandler(II, Context);
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

RSPragmaHandler* RSPragmaHandler::CreatePragmaJavaPackageNameHandler(RSContext* Context) {
    IdentifierInfo* II = Context->getPreprocessor()->getIdentifierInfo("java_package_name");
    if(II != NULL)
        return new RSJavaPackageNamePragmaHandler(II, Context);
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

void RSPragmaHandler::handleNonParamPragma(Preprocessor& PP, Token& FirstToken) {
    Token& PragmaToken = FirstToken;

    /* Skip first token, like "export_var_all" */
    PP.LexUnexpandedToken(PragmaToken);

    /* Should be end immediately */
    if(PragmaToken.isNot(tok::eom))
        printf("RSPragmaHandler::handleNonParamPragma: expected a tok::eom\n");
    return;
}

}   /* namespace slang */
