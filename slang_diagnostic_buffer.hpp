#ifndef _SLANG_DIAGNOSTIC_BUFFER_HPP
#   define _SLANG_DIAGNOSTIC_BUFFER_HPP

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
  llvm::raw_string_ostream* mSOS;

 public:
  DiagnosticBuffer();

  virtual void HandleDiagnostic(clang::Diagnostic::Level DiagLevel,
                                const clang::DiagnosticInfo& Info);

  inline const std::string &str() const { mSOS->flush(); return mDiags; }

  inline void reset() { this->mSOS->str().clear(); return; }

  virtual ~DiagnosticBuffer();
};
}

#endif  // _SLANG_DIAGNOSTIC_BUFFER_HPP
