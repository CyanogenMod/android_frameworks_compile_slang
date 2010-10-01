#include "slang_diagnostic_buffer.hpp"

#include "llvm/ADT/SmallString.h"

#include "clang/Basic/SourceManager.h"
#include "clang/Basic/SourceLocation.h"

using namespace slang;

DiagnosticBuffer::DiagnosticBuffer() : mSOS(NULL) {
  mSOS = new llvm::raw_string_ostream(mDiags);
  return;
}

void DiagnosticBuffer::HandleDiagnostic(clang::Diagnostic::Level DiagLevel,
                                        const clang::DiagnosticInfo &Info) {
  const clang::FullSourceLoc &FSLoc = Info.getLocation();
  // 100 is enough for storing general diagnosis message
  llvm::SmallString<100> Buf;

  if (FSLoc.isValid()) {
    FSLoc.print(*mSOS, FSLoc.getManager());
    (*mSOS) << ": ";
  }

  switch (DiagLevel) {
    case clang::Diagnostic::Note: {
      (*mSOS) << "note: ";
      break;
    }
    case clang::Diagnostic::Warning: {
      (*mSOS) << "warning: ";
      break;
    }
    case clang::Diagnostic::Error: {
      (*mSOS) << "error: ";
      break;
    }
    case clang::Diagnostic::Fatal: {
      (*mSOS) << "fatal: ";
      break;
    }
    default: {
      assert(0 && "Diagnostic not handled during diagnostic buffering!");
    }
  }


  Info.FormatDiagnostic(Buf);
  (*mSOS) << Buf.str() << '\n';

  return;
}

DiagnosticBuffer::~DiagnosticBuffer() {
  if (mSOS != NULL)
    delete mSOS;
  return;
}
