#ifndef _SLANG_DIAGNOSTIC_BUFFER_HPP
#   define _SLANG_DIAGNOSTIC_BUFFER_HPP

#include <string>

#include "llvm/Support/raw_ostream.h"       /* for class llvm::raw_ostream and llvm::raw_string_ostream */

#include "clang/Basic/Diagnostic.h"     /* for class clang::Diagnostic, class clang::DiagnosticClient, class clang::DiagnosticInfo  */

namespace llvm {

class raw_string_ostream;

}   /* namespace llvm */

namespace slang {

using namespace clang;

/* The diagnostics client instance (for reading the processed diagnostics) */
class DiagnosticBuffer : public DiagnosticClient {
private:
    std::string mDiags;
    llvm::raw_string_ostream* mSOS;

public:
    DiagnosticBuffer();

    virtual void HandleDiagnostic(Diagnostic::Level DiagLevel, const DiagnosticInfo& Info);

    inline const std::string& str() const { mSOS->flush(); return mDiags; }

    inline void reset() { this->mSOS->str().clear(); return; }

    virtual ~DiagnosticBuffer();
};  /* class DiagnosticBuffer */


}   /* namespace slang */

#endif  /* _SLANG_DIAGNOSTIC_BUFFER_HPP */
