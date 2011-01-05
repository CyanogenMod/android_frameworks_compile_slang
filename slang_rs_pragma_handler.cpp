/*
 * Copyright 2010, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "slang_rs_pragma_handler.h"

#include <string>

#include "clang/Basic/TokenKinds.h"

#include "clang/Lex/LiteralSupport.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"

#include "slang_rs_context.h"

namespace slang {

namespace {  // Anonymous namespace

class RSExportTypePragmaHandler : public RSPragmaHandler {
 private:
  void handleItem(const std::string &Item) {
    mContext->addExportType(Item);
  }

 public:
  RSExportTypePragmaHandler(llvm::StringRef Name, RSContext *Context)
      : RSPragmaHandler(Name, Context) { return; }

  void HandlePragma(clang::Preprocessor &PP, clang::Token &FirstToken) {
    this->handleItemListPragma(PP, FirstToken);
  }
};

class RSJavaPackageNamePragmaHandler : public RSPragmaHandler {
 public:
  RSJavaPackageNamePragmaHandler(llvm::StringRef Name, RSContext *Context)
      : RSPragmaHandler(Name, Context) { return; }

  void HandlePragma(clang::Preprocessor &PP, clang::Token &FirstToken) {
    // FIXME: Need to validate the extracted package name from pragma.
    // Currently "all chars" specified in pragma will be treated as package
    // name.
    //
    // 18.1 The Grammar of the Java Programming Language
    // (http://java.sun.com/docs/books/jls/third_edition/html/syntax.html#18.1)
    //
    // CompilationUnit:
    //     [[Annotations] package QualifiedIdentifier   ;  ] {ImportDeclaration}
    //     {TypeDeclaration}
    //
    // QualifiedIdentifier:
    //     Identifier { . Identifier }
    //
    // Identifier:
    //     IDENTIFIER
    //
    // 3.8 Identifiers
    // (http://java.sun.com/docs/books/jls/third_edition/html/lexical.html#3.8)
    //
    //

    clang::Token &PragmaToken = FirstToken;
    std::string PackageName;

    // Skip first token, "java_package_name"
    PP.LexUnexpandedToken(PragmaToken);

    // Now, the current token must be clang::tok::lpara
    if (PragmaToken.isNot(clang::tok::l_paren))
      return;

    while (PragmaToken.isNot(clang::tok::eom)) {
      // Lex package name
      PP.LexUnexpandedToken(PragmaToken);

      bool Invalid;
      std::string Spelling = PP.getSpelling(PragmaToken, &Invalid);
      if (!Invalid)
        PackageName.append(Spelling);

      // Pre-mature end (syntax error will be triggered by preprocessor later)
      if (PragmaToken.is(clang::tok::eom) || PragmaToken.is(clang::tok::eof)) {
        break;
      } else {
        // Next token is ')' (end of paragma)
        const clang::Token &NextTok = PP.LookAhead(0);
        if (NextTok.is(clang::tok::r_paren)) {
          mContext->setReflectJavaPackageName(PackageName);
          // Lex until meets clang::tok::eom
          do {
            PP.LexUnexpandedToken(PragmaToken);
          } while (PragmaToken.isNot(clang::tok::eom));
          break;
        }
      }
    }
    return;
  }
};

class RSReflectLicensePragmaHandler : public RSPragmaHandler {
 private:
  void handleItem(const std::string &Item) {
    mContext->setLicenseNote(Item);
  }

 public:
  RSReflectLicensePragmaHandler(llvm::StringRef Name, RSContext *Context)
      : RSPragmaHandler(Name, Context) { return; }

  void HandlePragma(clang::Preprocessor &PP, clang::Token &FirstToken) {
    this->handleOptionalStringLiteralParamPragma(PP, FirstToken);
  }
};

}  // namespace

RSPragmaHandler *
RSPragmaHandler::CreatePragmaExportTypeHandler(RSContext *Context) {
  return new RSExportTypePragmaHandler("export_type", Context);
}

RSPragmaHandler *
RSPragmaHandler::CreatePragmaJavaPackageNameHandler(RSContext *Context) {
  return new RSJavaPackageNamePragmaHandler("java_package_name", Context);
}

RSPragmaHandler *
RSPragmaHandler::CreatePragmaReflectLicenseHandler(RSContext *Context) {
  return new RSJavaPackageNamePragmaHandler("set_reflect_license", Context);
}

void RSPragmaHandler::handleItemListPragma(clang::Preprocessor &PP,
                                           clang::Token &FirstToken) {
  clang::Token &PragmaToken = FirstToken;

  // Skip first token, like "export_var"
  PP.LexUnexpandedToken(PragmaToken);

  // Now, the current token must be clang::tok::lpara
  if (PragmaToken.isNot(clang::tok::l_paren))
    return;

  while (PragmaToken.isNot(clang::tok::eom)) {
    // Lex variable name
    PP.LexUnexpandedToken(PragmaToken);
    if (PragmaToken.is(clang::tok::identifier))
      this->handleItem(PP.getSpelling(PragmaToken));
    else
      break;

    assert(PragmaToken.isNot(clang::tok::eom));

    PP.LexUnexpandedToken(PragmaToken);

    if (PragmaToken.isNot(clang::tok::comma))
      break;
  }
  return;
}

void RSPragmaHandler::handleNonParamPragma(clang::Preprocessor &PP,
                                           clang::Token &FirstToken) {
  clang::Token &PragmaToken = FirstToken;

  // Skip first token, like "export_var_all"
  PP.LexUnexpandedToken(PragmaToken);

  // Should be end immediately
  if (PragmaToken.isNot(clang::tok::eom))
    fprintf(stderr, "RSPragmaHandler::handleNonParamPragma: "
                    "expected a clang::tok::eom\n");
  return;
}

void RSPragmaHandler::handleOptionalStringLiteralParamPragma(
    clang::Preprocessor &PP, clang::Token &FirstToken) {
  clang::Token &PragmaToken = FirstToken;

  // Skip first token, like "set_reflect_license"
  PP.LexUnexpandedToken(PragmaToken);

  // Now, the current token must be clang::tok::lpara
  if (PragmaToken.isNot(clang::tok::l_paren))
    return;

  // If not ')', eat the following string literal as the license
  PP.LexUnexpandedToken(PragmaToken);
  if (PragmaToken.isNot(clang::tok::r_paren)) {
    // Eat the whole string literal
    clang::StringLiteralParser StringLiteral(&PragmaToken, 1, PP);
    if (StringLiteral.hadError)
      fprintf(stderr, "RSPragmaHandler::handleOptionalStringLiteralParamPragma"
                      ": illegal string literal\n");
    else
      this->handleItem(std::string(StringLiteral.GetString()));

    // The current token should be clang::tok::r_para
    PP.LexUnexpandedToken(PragmaToken);
    if (PragmaToken.isNot(clang::tok::r_paren))
      fprintf(stderr, "RSPragmaHandler::handleOptionalStringLiteralParamPragma"
                      ": expected a ')'\n");
  } else {
    // If no argument, remove the license
    this->handleItem("");
  }
}

}  // namespace slang
