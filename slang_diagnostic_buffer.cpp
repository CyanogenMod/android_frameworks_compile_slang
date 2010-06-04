#include "slang_diagnostic_buffer.hpp"

#include "llvm/ADT/SmallString.h"   /* for llvm::SmallString */

#include "clang/Basic/SourceManager.h"  /* for class clang::SourceManager */
#include "clang/Basic/SourceLocation.h" /* for class clang::SourceLocation, class clang::FullSourceLoc and class clang::PresumedLoc */

namespace slang {

DiagnosticBuffer::DiagnosticBuffer() : mSOS(NULL) {
    mSOS = new llvm::raw_string_ostream(mDiags);
    return;
}

void DiagnosticBuffer::HandleDiagnostic(Diagnostic::Level DiagLevel, const DiagnosticInfo &Info) {
    const FullSourceLoc& FSLoc = Info.getLocation();
    llvm::SmallString<100> Buf; /* 100 is enough for storing general diagnosis message */

    if(FSLoc.isValid()) {
        /* This is a diagnosis for a source code */
        PresumedLoc PLoc = FSLoc.getManager().getPresumedLoc(FSLoc);
        (*mSOS) << FSLoc.getManager().getBufferName(FSLoc) << ':' << PLoc.getLine() << ':' << PLoc.getColumn() << ": ";
    }

    switch(DiagLevel) {
        case Diagnostic::Note:
            (*mSOS) << "note: ";
        break;

        case Diagnostic::Warning:
            (*mSOS) << "warning: ";
        break;

        case Diagnostic::Error:
            (*mSOS) << "error: ";
        break;

        case Diagnostic::Fatal:
            (*mSOS) << "fatal: ";
        break;

        default:
            assert(0 && "Diagnostic not handled during diagnostic buffering!");
        break;
    }


    Info.FormatDiagnostic(Buf);
    (*mSOS) << Buf.str() << '\n';

    return;
}

DiagnosticBuffer::~DiagnosticBuffer() {
    if(mSOS != NULL)
        delete mSOS;
    return;
}

}   /* namespace slang */
