#include "slang_rs_context.hpp"
#include "slang_rs_pragma_handler.hpp"

#include "clang/Lex/Preprocessor.h"         /* for class Preprocessor */
#include "clang/Lex/Token.h"                /* for class Token */
#include "clang/Lex/LiteralSupport.h"       /* for class StringLiteralParser */
#include "clang/Basic/TokenKinds.h"         /* for class Token */

namespace {

using namespace clang;
using namespace slang;

class RSExportVarPragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->addExportVar(Item);
    }

public:
    RSExportVarPragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleItemListPragma(PP, FirstToken);
    }
};

class RSExportVarAllPragmaHandler : public RSPragmaHandler {
public:
    RSExportVarAllPragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleNonParamPragma(PP, FirstToken);
        mContext->setExportAllNonStaticVars(true);
    }
};

class RSExportFuncPragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->addExportFunc(Item);
    }

public:
    RSExportFuncPragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleItemListPragma(PP, FirstToken);
    }
};

class RSExportFuncAllPragmaHandler : public RSPragmaHandler {
public:
    RSExportFuncAllPragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleNonParamPragma(PP, FirstToken);
        mContext->setExportAllNonStaticFuncs(true);
    }
};

class RSExportTypePragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->addExportType(Item);
    }

public:
    RSExportTypePragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleItemListPragma(PP, FirstToken);
    }
};

class RSJavaPackageNamePragmaHandler : public RSPragmaHandler {
public:
    RSJavaPackageNamePragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

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

class RSReflectLicensePragmaHandler : public RSPragmaHandler {
private:
    void handleItem(const std::string& Item) {
        mContext->setLicenseNote(Item);
    }

public:
  RSReflectLicensePragmaHandler(llvm::StringRef Name, RSContext* Context) : RSPragmaHandler(Name, Context) { return; }

    void HandlePragma(Preprocessor& PP, Token& FirstToken) {
        this->handleOptionalStringLiateralParamPragma(PP, FirstToken);
    }
};

}   /* anonymous namespace */

namespace slang {

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportVarHandler(RSContext* Context) {
    return new RSExportVarPragmaHandler("export_var", Context);
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportVarAllHandler(RSContext* Context) {
    return new RSExportVarPragmaHandler("export_var_all", Context);
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportFuncHandler(RSContext* Context) {
    return new RSExportFuncPragmaHandler("export_func", Context);
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportFuncAllHandler(RSContext* Context) {
    return new RSExportFuncPragmaHandler("export_func_all", Context);
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaExportTypeHandler(RSContext* Context) {
    return new RSExportTypePragmaHandler("export_type", Context);
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaJavaPackageNameHandler(RSContext* Context) {
    return new RSJavaPackageNamePragmaHandler("java_package_name", Context);
}

RSPragmaHandler* RSPragmaHandler::CreatePragmaReflectLicenseHandler(RSContext* Context) {
    return new RSJavaPackageNamePragmaHandler("set_reflect_license", Context);
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

void RSPragmaHandler::handleOptionalStringLiateralParamPragma(Preprocessor& PP, Token& FirstToken) {
    Token& PragmaToken = FirstToken;

    /* Skip first token, like "set_reflect_license" */
    PP.LexUnexpandedToken(PragmaToken);

    /* Now, the current token must be tok::lpara */
    if(PragmaToken.isNot(tok::l_paren))
        return;

    /* If not ')', eat the following string literal as the license */
    PP.LexUnexpandedToken(PragmaToken);
    if(PragmaToken.isNot(tok::r_paren)) {
        /* Eat the whole string literal */
        StringLiteralParser StringLiteral(&PragmaToken, 1, PP);
        if (StringLiteral.hadError)
            printf("RSPragmaHandler::handleOptionalStringLiateralParamPragma: illegal string literal\n");
        else
            this->handleItem( std::string(StringLiteral.GetString()) );

        /* The current token should be tok::r_para */
        PP.LexUnexpandedToken(PragmaToken);
        if (PragmaToken.isNot(tok::r_paren))
            printf("RSPragmaHandler::handleOptionalStringLiateralParamPragma: expected a ')'\n");
    } else {
        /* If no argument, remove the license */
        this->handleItem( "" );
    }
}

}   /* namespace slang */
