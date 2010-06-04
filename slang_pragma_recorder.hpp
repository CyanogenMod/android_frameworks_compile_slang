#ifndef _SLANG_COMPILER_PRAGMA_HANDLER_HPP
#   define _SLANG_COMPILER_PRAGMA_HANDLER_HPP

#include <list>
#include <string>

#include "clang/Lex/Pragma.h"           /* for class clang::PragmaHandler */

namespace clang {

class Token;
class Preprocessor;

}   /* namespace clang */

namespace slang {

using namespace clang;

typedef std::list< std::pair<std::string, std::string> > PragmaList;

class PragmaRecorder : public PragmaHandler {
private:
    PragmaList& mPragmas;

    static bool GetPragmaNameFromToken(const Token& Token, std::string& PragmaName);

    static bool GetPragmaValueFromToken(const Token& Token, std::string& PragmaValue);

public:
    PragmaRecorder(PragmaList& Pragmas);

    virtual void HandlePragma(Preprocessor &PP, Token &FirstToken);
};  /* class PragmaRecorder */

}   /* namespace slang */

#endif  /* _SLANG_COMPILER_PRAGMA_HANDLER_HPP */
