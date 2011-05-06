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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_ROOT_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_ROOT_H_

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/AST/Decl.h"

#include "slang_assert.h"
#include "slang_rs_context.h"

namespace clang {
  class FunctionDecl;
}  // namespace clang

namespace slang {

// Base class for handling root() functions (including reflection of
// control-side code for issuing rsForEach).
class RSRoot {
 private:
  std::string mName;
  std::string mMangledName;
  bool mShouldMangle;

  RSRoot(RSContext *Context, const llvm::StringRef &Name,
         const clang::FunctionDecl *FD)
    : mName(Name.data(), Name.size()),
      mMangledName(),
      mShouldMangle(false) {
    mShouldMangle = Context->getMangleContext().shouldMangleDeclName(FD);

    if (mShouldMangle) {
      llvm::raw_string_ostream BufStm(mMangledName);
      Context->getMangleContext().mangleName(FD, BufStm);
      BufStm.flush();
    }

    return;
  }

 public:
  static RSRoot *Create(RSContext *Context, const clang::FunctionDecl *FD);

  inline const std::string &getName(bool mangle = true) const {
    return (mShouldMangle && mangle) ? mMangledName : mName;
  }

  inline static bool isInitRSFunc(const clang::FunctionDecl *FD) {
    if (!FD) {
      return false;
    }
    const llvm::StringRef Name = FD->getName();
    static llvm::StringRef FuncInit("init");
    return Name.equals(FuncInit);
  }

  inline static bool isRootRSFunc(const clang::FunctionDecl *FD) {
    if (!FD) {
      return false;
    }
    const llvm::StringRef Name = FD->getName();
    static llvm::StringRef FuncRoot("root");
    return Name.equals(FuncRoot);
  }

  inline static bool isSpecialRSFunc(const clang::FunctionDecl *FD) {
    return isRootRSFunc(FD) || isInitRSFunc(FD);
  }

  static bool validateSpecialFuncDecl(clang::Diagnostic *Diags,
                                      const clang::FunctionDecl *FD);
};  // RSRoot

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_ROOT_H_  NOLINT
