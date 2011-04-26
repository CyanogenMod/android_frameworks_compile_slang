/*
 * Copyright 2011, The Android Open Source Project
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

#include "slang_rs_root.h"

#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Target/TargetData.h"

#include "slang_assert.h"
#include "slang_rs_context.h"

namespace slang {

RSRoot *RSRoot::Create(RSContext *Context, const clang::FunctionDecl *FD) {
  llvm::StringRef Name = FD->getName();
  RSRoot *F;

  slangAssert(!Name.empty() && "Function must have a name");

  F = new RSRoot(Context, Name, FD);

  return F;
}

bool RSRoot::validateSpecialFuncDecl(clang::Diagnostic *Diags,
                                     const clang::FunctionDecl *FD) {
  if (!FD) {
    return false;
  }

  bool valid = true;
  const clang::ASTContext &C = FD->getASTContext();

  if (isRootRSFunc(FD)) {
    if (FD->getNumParams() == 0) {
      // Graphics root function, so verify that it returns an int
      if (FD->getResultType().getCanonicalType() != C.IntTy) {
        Diags->Report(
            clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
            Diags->getCustomDiagID(clang::Diagnostic::Error,
                                   "root(void) is required to return "
                                   "an int for graphics usage"));
        valid = false;
      }
    } else {
      // Compute root functions are required to return a void type for now
      if (FD->getResultType().getCanonicalType() != C.VoidTy) {
        Diags->Report(
            clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
            Diags->getCustomDiagID(clang::Diagnostic::Error,
                                   "compute root() is required to return a "
                                   "void type"));
        valid = false;
      }

      // Validate remaining parameter types
      // TODO(srhines)
    }
  }
  else if (isInitRSFunc(FD)) {
    if (FD->getNumParams() != 0) {
      Diags->Report(
          clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
          Diags->getCustomDiagID(clang::Diagnostic::Error,
                                 "init(void) is required to have no "
                                 "parameters"));
      valid = false;
    }

    if (FD->getResultType().getCanonicalType() != C.VoidTy) {
      Diags->Report(
          clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
          Diags->getCustomDiagID(clang::Diagnostic::Error,
                                 "init(void) is required to have a void "
                                 "return type"));
      valid = false;
    }
  }
  else {
    slangAssert(false && "must be called on init or root function!");
  }

  return valid;
}

}  // namespace slang
