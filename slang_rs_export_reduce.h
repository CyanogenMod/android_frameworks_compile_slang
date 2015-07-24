/*
 * Copyright 2015, The Android Open Source Project
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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_EXPORT_REDUCE_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_EXPORT_REDUCE_H_

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

#include "slang_rs_context.h"
#include "slang_rs_exportable.h"
#include "slang_rs_export_type.h"

namespace clang {
  class FunctionDecl;
}  // namespace clang

namespace slang {

// Base class for reflecting control-side reduce
class RSExportReduce : public RSExportable {
 public:
  typedef llvm::SmallVectorImpl<const clang::ParmVarDecl*> InVec;
  typedef InVec::const_iterator InIter;

 private:
  // Function name
  std::string mName;
  // Input and output type
  RSExportType *mType;
  // Inputs
  llvm::SmallVector<const clang::ParmVarDecl *, 2> mIns;

  RSExportReduce(RSContext *Context, const llvm::StringRef &Name)
    : RSExportable(Context, RSExportable::EX_REDUCE),
      mName(Name.data(), Name.size()), mType(nullptr), mIns(2) {
  }

  RSExportReduce(const RSExportReduce &) = delete;
  RSExportReduce &operator=(const RSExportReduce &) = delete;

  // Given a reduce kernel declaration, validate the parameters to the
  // reduce kernel.
  bool validateAndConstructParams(RSContext *Context,
                                  const clang::FunctionDecl *FD);

 public:
  static RSExportReduce *Create(RSContext *Context,
                                const clang::FunctionDecl *FD);

  const std::string &getName() const {
    return mName;
  }

  const InVec &getIns() const {
    return mIns;
  }

  const RSExportType *getType() const {
    return mType;
  }

  static bool isRSReduceFunc(unsigned int targetAPI,
                             const clang::FunctionDecl *FD);

};  // RSExportReduce

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_EXPORT_REDUCE_H_  NOLINT
