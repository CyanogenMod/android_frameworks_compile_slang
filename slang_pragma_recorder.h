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

#ifndef _SLANG_COMPILER_PRAGMA_HANDLER_H
#define _SLANG_COMPILER_PRAGMA_HANDLER_H

#include <list>
#include <string>

#include "clang/Lex/Pragma.h"

namespace clang {
  class Token;
  class Preprocessor;
}

namespace slang {

typedef std::list< std::pair<std::string, std::string> > PragmaList;

class PragmaRecorder : public clang::PragmaHandler {
 private:
  PragmaList &mPragmas;

  static bool GetPragmaNameFromToken(const clang::Token &Token,
                                     std::string &PragmaName);

  static bool GetPragmaValueFromToken(const clang::Token &Token,
                                      std::string &PragmaValue);

 public:
  explicit PragmaRecorder(PragmaList &Pragmas);

  virtual void HandlePragma(clang::Preprocessor &PP,
                            clang::Token &FirstToken);
};
}

#endif  // _SLANG_COMPILER_PRAGMA_HANDLER_H
