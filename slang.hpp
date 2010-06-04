#ifndef _SLANG_COMPILER_SLANG_HPP
#   define _SLANG_COMPILER_SLANG_HPP

#include "slang_backend.hpp"
#include "slang_rs_context.hpp"
#include "slang_rs_backend.hpp"
#include "slang_pragma_recorder.hpp"
#include "slang_diagnostic_buffer.hpp"

#include <cstdio>
#include <string>

#include "llvm/Support/raw_ostream.h"   /* for class llvm::raw_ostream */

#include "llvm/ADT/OwningPtr.h"         /* for class llvm::OwningPtr */
#include "llvm/ADT/StringRef.h"         /* for class llvm::StringRef */

#include "clang/AST/ASTConsumer.h"      /* for class clang::ASTConsumer */
#include "clang/AST/ASTContext.h"       /* for class clang::ASTContext */

#include "clang/Lex/Preprocessor.h"     /* for class clang::Preprocessor */
#include "clang/Lex/HeaderSearch.h"     /* for class clang::HeaderSearch */

#include "clang/Basic/Diagnostic.h"     /* for class clang::Diagnostic, class clang::DiagnosticClient, class clang::DiagnosticInfo  */
#include "clang/Basic/FileManager.h"    /* for class clang::FileManager and class clang::FileEntry */
#include "clang/Basic/TargetOptions.h"  /* for class clang::TargetOptions */

namespace llvm {

class Twine;
class TargetInfo;

}   /* namespace llvm */

namespace clang {

class LangOptions;
class CodeGenOptions;

}   /* namespace clang */

namespace slang {

using namespace clang;

class Slang {
    static LangOptions LangOpts;
    static CodeGenOptions CodeGenOpts;

    static bool GlobalInitialized;

    static void GlobalInitialization();

    static void LLVMErrorHandler(void *UserData, const std::string &Message);

private:
    PragmaList mPragmas;

    /* The diagnostics engine instance (for status reporting during compilation) */
    llvm::OwningPtr<Diagnostic> mDiagnostics;

    llvm::OwningPtr<DiagnosticBuffer> mDiagClient;
    inline void createDiagnostic() {
        mDiagClient.reset(new DiagnosticBuffer());
        mDiagnostics.reset(new Diagnostic(mDiagClient.get()));
        return;
    }

    /* The target being compiled for */
    TargetOptions mTargetOpts;
    llvm::OwningPtr<TargetInfo> mTarget;
    void createTarget(const char* Triple, const char* CPU, const char** Features);

    /**** Below is for parsing ****/

    /* The file manager (for prepocessor doing the job such as header file search) */
    llvm::OwningPtr<FileManager> mFileMgr;
    inline void createFileManager() { mFileMgr.reset(new FileManager()); return; }

    /* The source manager (responsible for the source code handling) */
    llvm::OwningPtr<SourceManager> mSourceMgr;  /* The source manager */
    inline void createSourceManager() { mSourceMgr.reset(new SourceManager(*mDiagnostics)); return; }

    /* The preprocessor (source code preprocessor) */
    llvm::OwningPtr<Preprocessor> mPP;
    inline void createPreprocessor() {
        HeaderSearch* HeaderInfo = new HeaderSearch(*mFileMgr); /* Default only search header file in current dir */
        mPP.reset(new Preprocessor( *mDiagnostics,
                                    LangOpts,
                                    *mTarget,
                                    *mSourceMgr,
                                    *HeaderInfo,
                                    NULL,
                                    true /* OwnsHeaderSearch */));
        /* Initialize the prepocessor */
        mPragmas.clear();
        mPP->AddPragmaHandler(NULL, new PragmaRecorder(mPragmas));
        /* ApplyHeaderSearchOptions */
        return;
    }

    /* Context of Slang compiler for RenderScript */
    llvm::OwningPtr<RSContext> mRSContext;
    inline void createRSContext() {
        mRSContext.reset(new RSContext(mPP.get(),
                                       mTarget.get()));
        return;
    }

    /* The AST context (the context to hold long-lived AST nodes) */
    llvm::OwningPtr<ASTContext> mASTContext;
    inline void createASTContext() {
        mASTContext.reset(new ASTContext(LangOpts,
                                         *mSourceMgr,
                                         *mTarget,
                                         mPP->getIdentifierTable(),
                                         mPP->getSelectorTable(),
                                         mPP->getBuiltinInfo()));
        return;
    }

    /* The AST consumer, responsible for code generation */
    llvm::OwningPtr<Backend> mBackend;
    inline void createBackend() {
        mBackend.reset(new Backend(*mDiagnostics,
                                   CodeGenOpts,
                                   mTargetOpts,
                                   mPragmas,
                                   mOS.take(),
                                   mOutputType));

        return;
    }

    inline void createRSBackend() {
        mBackend.reset(new RSBackend(mRSContext.get(),
                                     *mDiagnostics,
                                     CodeGenOpts,
                                     mTargetOpts,
                                     mPragmas,
                                     mOS.take(),
                                     mOutputType));

        return;
    }

    /* Input file name */
    std::string mInputFileName;
    std::string mOutputFileName;

    SlangCompilerOutputTy mOutputType;

    /* Output stream */
    llvm::OwningPtr<llvm::raw_ostream> mOS;

public:
    static const std::string TargetDescription;

    static const llvm::Twine PragmaMetadataName;

    Slang(const char* Triple, const char* CPU, const char** Features);

    bool setInputSource(llvm::StringRef inputFile, const char* text, size_t textLength);

    bool setInputSource(llvm::StringRef inputFile);

    void setOutputType(SlangCompilerOutputTy outputType);

    inline bool setOutput(FILE* stream) {
        if(stream == NULL)
            return false;

        mOS.reset( new llvm::raw_fd_ostream(fileno(stream), /* shouldClose */false) );
        return true;
    }

    bool setOutput(const char* outputFile);

    int compile();

    bool reflectToJava(const char* outputPackageName);

    inline const char* getErrorMessage() {
        return mDiagClient->str().c_str();
    }

    void getPragmas(size_t* actualStringCount, size_t maxStringCount, char** strings);

    /* Reset the slang compiler state such that it can be reused to compile another file */
    inline void reset() {
        /* Seems there's no way to clear the diagnostics. We just re-create it. */
        createDiagnostic();
        mOutputType = SlangCompilerOutput_Default;
        return;
    }

    ~Slang();
};  /* class Slang */

}   /* namespace slang */

#endif  /* _SLANG_COMPILER_SLANG_HPP */
