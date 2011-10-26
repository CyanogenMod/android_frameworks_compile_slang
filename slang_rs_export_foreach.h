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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_EXPORT_FOREACH_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_EXPORT_FOREACH_H_

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/AST/Decl.h"

#include "slang_assert.h"
#include "slang_rs_context.h"
#include "slang_rs_exportable.h"
#include "slang_rs_export_type.h"

namespace clang {
  class FunctionDecl;
}  // namespace clang

namespace slang {

// Base class for reflecting control-side forEach (currently for root()
// functions that fit appropriate criteria)
class RSExportForEach : public RSExportable {
 private:
  std::string mName;
  RSExportRecordType *mParamPacketType;
  RSExportType *mInType;
  RSExportType *mOutType;
  size_t numParams;

  unsigned int mMetadataEncoding;

  const clang::ParmVarDecl *mIn;
  const clang::ParmVarDecl *mOut;
  const clang::ParmVarDecl *mUsrData;
  const clang::ParmVarDecl *mX;
  const clang::ParmVarDecl *mY;
  const clang::ParmVarDecl *mZ;
  const clang::ParmVarDecl *mAr;

  // TODO(all): Add support for LOD/face when we have them
  RSExportForEach(RSContext *Context, const llvm::StringRef &Name,
         const clang::FunctionDecl *FD)
    : RSExportable(Context, RSExportable::EX_FOREACH),
      mName(Name.data(), Name.size()), mParamPacketType(NULL), mInType(NULL),
      mOutType(NULL), numParams(0), mMetadataEncoding(0),
      mIn(NULL), mOut(NULL), mUsrData(NULL),
      mX(NULL), mY(NULL), mZ(NULL), mAr(NULL) {
    return;
  }

  bool validateAndConstructParams(RSContext *Context,
                                  const clang::FunctionDecl *FD);

 public:
  static RSExportForEach *Create(RSContext *Context,
                                 const clang::FunctionDecl *FD);

  inline const std::string &getName() const {
    return mName;
  }

  inline size_t getNumParameters() const {
    return numParams;
  }

  inline bool hasIn() const {
    return (mIn != NULL);
  }

  inline bool hasOut() const {
    return (mOut != NULL);
  }

  inline bool hasUsrData() const {
    return (mUsrData != NULL);
  }

  inline const RSExportType *getInType() const {
    return mInType;
  }

  inline const RSExportType *getOutType() const {
    return mOutType;
  }

  inline const RSExportRecordType *getParamPacketType() const {
    return mParamPacketType;
  }

  inline unsigned int getMetadataEncoding() const {
    return mMetadataEncoding;
  }

  typedef RSExportRecordType::const_field_iterator const_param_iterator;

  inline const_param_iterator params_begin() const {
    slangAssert((mParamPacketType != NULL) &&
                "Get parameter from export foreach having no parameter!");
    return mParamPacketType->fields_begin();
  }

  inline const_param_iterator params_end() const {
    slangAssert((mParamPacketType != NULL) &&
                "Get parameter from export foreach having no parameter!");
    return mParamPacketType->fields_end();
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

  inline static bool isDtorRSFunc(const clang::FunctionDecl *FD) {
    if (!FD) {
      return false;
    }
    const llvm::StringRef Name = FD->getName();
    static llvm::StringRef FuncDtor(".rs.dtor");
    return Name.equals(FuncDtor);
  }

  static bool isRSForEachFunc(int targetAPI, const clang::FunctionDecl *FD);

  inline static bool isSpecialRSFunc(const clang::FunctionDecl *FD) {
    return isRootRSFunc(FD) || isInitRSFunc(FD) || isDtorRSFunc(FD);
  }

  static bool validateSpecialFuncDecl(int targetAPI,
                                      clang::Diagnostic *Diags,
                                      const clang::FunctionDecl *FD);
};  // RSExportForEach

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_EXPORT_FOREACH_H_  NOLINT
