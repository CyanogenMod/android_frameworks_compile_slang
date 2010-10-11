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

#ifndef _SLANG_COMPILER_DIAGNOSTIC_BUFFER_H
#define _SLANG_COMPILER_DIAGNOSTIC_BUFFER_H

#include "llvm/Support/raw_ostream.h"

#include "clang/Basic/Diagnostic.h"

#include <string>

namespace llvm {
  class raw_string_ostream;
}

namespace slang {

// The diagnostics client instance (for reading the processed diagnostics)
class DiagnosticBuffer : public clang::DiagnosticClient {
 private:
  std::string mDiags;
  llvm::raw_string_ostream *mSOS;

 public:
  DiagnosticBuffer();

  virtual void HandleDiagnostic(clang::Diagnostic::Level DiagLevel,
                                const clang::DiagnosticInfo& Info);

  inline const std::string &str() const {
    mSOS->flush();
    return mDiags;
  }

  inline void reset() {
    this->mSOS->str().clear();
    return;
  }

  virtual ~DiagnosticBuffer();
};
}

#endif  // _SLANG_DIAGNOSTIC_BUFFER_H
