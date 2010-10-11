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

#ifndef _SLANG_COMPILER_RS_EXPORTABLE_HPP
#define _SLANG_COMPILER_RS_EXPORTABLE_HPP

#include "slang_rs_context.h"

namespace slang {

class RSExportable {
 public:
  enum Kind {
    EX_FUNC,
    EX_TYPE,
    EX_VAR
  };

 private:
  Kind mK;

 protected:
  RSExportable(RSContext *Context, RSExportable::Kind K) : mK(K) {
    Context->newExportable(this);
    return;
  }

 public:
  inline Kind getKind() const { return mK; }

  virtual ~RSExportable() { }
};
}

#endif  // _SLANG_COMPILER_RS_EXPORTABLE_HPP
