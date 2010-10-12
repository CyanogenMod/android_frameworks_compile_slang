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

#ifndef _SLANG_COMPILER_RS_EXPORT_FUNC_H
#define _SLANG_COMPILER_RS_EXPORT_FUNC_H

#include <list>
#include <string>

#include "llvm/ADT/StringRef.h"

#include "slang_rs_exportable.h"
#include "slang_rs_export_type.h"

namespace llvm {
  class StructType;
}

namespace clang {
  class FunctionDecl;
}   // namespace clang

namespace slang {

class RSContext;

class RSExportFunc : public RSExportable {
  friend class RSContext;

 private:
  std::string mName;
  RSExportRecordType *mParamPacketType;

  RSExportFunc(RSContext *Context, const llvm::StringRef &Name)
    : RSExportable(Context, RSExportable::EX_FUNC),
      mName(Name.data(), Name.size()),
      mParamPacketType(NULL) {
    return;
  }

 public:
  static RSExportFunc *Create(RSContext *Context,
                              const clang::FunctionDecl *FD);

  typedef RSExportRecordType::const_field_iterator const_param_iterator;

  inline const_param_iterator params_begin() const {
    assert((mParamPacketType != NULL) &&
           "Get parameter from export function having no parameter!");
    return mParamPacketType->fields_begin();
  }
  inline const_param_iterator params_end() const {
    assert((mParamPacketType != NULL) &&
           "Get parameter from export function having no parameter!");
    return mParamPacketType->fields_end();
  }

  inline const std::string &getName() const { return mName; }

  inline bool hasParam() const
    { return (mParamPacketType && !mParamPacketType->getFields().empty()); }
  inline size_t getNumParameters() const
    { return ((mParamPacketType) ? mParamPacketType->getFields().size() : 0); }

  inline const RSExportRecordType *getParamPacketType() const
    { return mParamPacketType; }

  // Check whether the given ParamsPacket type (in LLVM type) is "size
  // equivalent" to the one obtained from getParamPacketType(). If the @Params
  // is NULL, means there must be no any parameters.
  bool checkParameterPacketType(const llvm::StructType *ParamTy) const;
};  // RSExportFunc


}   // namespace slang

#endif  // _SLANG_COMPILER_RS_EXPORT_FUNC_H
