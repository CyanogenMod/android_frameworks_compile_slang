#ifndef _SLANG_COMPILER_PRAGMA_HANDLER_HPP
#   define _SLANG_COMPILER_PRAGMA_HANDLER_HPP

#include "clang/Lex/Pragma.h"

#include <list>
#include <string>


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
  PragmaRecorder(PragmaList &Pragmas);

  virtual void HandlePragma(clang::Preprocessor &PP,
                            clang::Token &FirstToken);
};

}

#endif  // _SLANG_COMPILER_PRAGMA_HANDLER_HPP
