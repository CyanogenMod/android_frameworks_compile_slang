#ifndef _SLANG_COMPILER_SLANG_HPP
#   define _SLANG_COMPILER_SLANG_HPP

#include "slang_backend.hpp"
#include "slang_rs_context.hpp"
#include "slang_rs_backend.hpp"
#include "slang_pragma_recorder.hpp"
#include "slang_diagnostic_buffer.hpp"

#include <vector>

#include "llvm/Support/raw_ostream.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringRef.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"

#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/HeaderSearch.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/TargetOptions.h"

#include <cstdio>
#include <string>

namespace llvm {
class TargetInfo;
}

namespace clang {
class LangOptions;
class CodeGenOptions;
class TargetOptions;
}

namespace slang {

class Slang {
  static clang::LangOptions LangOpts;
  static clang::CodeGenOptions CodeGenOpts;

  static bool GlobalInitialized;

  static void GlobalInitialization();

  static void LLVMErrorHandler(void *UserData, const std::string &Message);

 private:
  PragmaList mPragmas;

  // The diagnostics engine instance (for status reporting during compilation)
  llvm::OwningPtr<clang::Diagnostic> mDiagnostics;

  llvm::OwningPtr<DiagnosticBuffer> mDiagClient;
  inline void createDiagnostic() {
    mDiagClient.reset(new DiagnosticBuffer());
    mDiagnostics.reset(new clang::Diagnostic(mDiagClient.get()));
    if (!mDiagnostics->setDiagnosticGroupMapping(
            "implicit-function-declaration",
            clang::diag::MAP_ERROR))
      assert("Unable find option group implicit-function-declaration");
    mDiagnostics->setDiagnosticMapping(
        clang::diag::ext_typecheck_convert_discards_qualifiers,
        clang::diag::MAP_ERROR);
    return;
  }

  // The target being compiled for
  clang::TargetOptions mTargetOpts;
  llvm::OwningPtr<clang::TargetInfo> mTarget;
  void createTarget(const char *Triple, const char *CPU, const char **Features);

  // Below is for parsing

  // The file manager (for prepocessor doing the job such as header file search)
  llvm::OwningPtr<clang::FileManager> mFileMgr;
  inline void createFileManager() {
    mFileMgr.reset(new clang::FileManager());
    return;
  }

  // The source manager (responsible for the source code handling)
  llvm::OwningPtr<clang::SourceManager> mSourceMgr;
  inline void createSourceManager() {
    mSourceMgr.reset(new clang::SourceManager(*mDiagnostics));
    return;
  }

  // The preprocessor (source code preprocessor)
  llvm::OwningPtr<clang::Preprocessor> mPP;
  void createPreprocessor();

  // The AST context (the context to hold long-lived AST nodes)
  llvm::OwningPtr<clang::ASTContext> mASTContext;
  inline void createASTContext() {
    mASTContext.reset(new clang::ASTContext(LangOpts,
                                            *mSourceMgr,
                                            *mTarget,
                                            mPP->getIdentifierTable(),
                                            mPP->getSelectorTable(),
                                            mPP->getBuiltinInfo(),
                                            /* size_reserve */0));
    return;
  }

  // Context for RenderScript
  llvm::OwningPtr<RSContext> mRSContext;
  inline void createRSContext() {
    mRSContext.reset(new RSContext(mPP.get(),
                                   mASTContext.get(),
                                   mTarget.get()));
    return;
  }

  // The AST consumer, responsible for code generation
  llvm::OwningPtr<Backend> mBackend;
  inline void createBackend() {
    mBackend.reset(new Backend(*mDiagnostics,
                               CodeGenOpts,
                               mTargetOpts,
                               mPragmas,
                               mOS.take(),
                               mOutputType,
                               *mSourceMgr,
                               mAllowRSPrefix));

    return;
  }

  inline void createRSBackend() {
    mBackend.reset(new RSBackend(mRSContext.get(),
                                 *mDiagnostics,
                                 CodeGenOpts,
                                 mTargetOpts,
                                 mPragmas,
                                 mOS.take(),
                                 mOutputType,
                                 *mSourceMgr,
                                 mAllowRSPrefix));

    return;
  }

  // Input file name
  std::string mInputFileName;
  std::string mOutputFileName;

  SlangCompilerOutputTy mOutputType;

  // Output stream
  llvm::OwningPtr<llvm::raw_ostream> mOS;

  bool mAllowRSPrefix;

  std::vector<std::string> mIncludePaths;

 public:
  static const std::string TargetDescription;

  static const llvm::StringRef PragmaMetadataName;

  Slang(const char *Triple, const char *CPU, const char **Features);

  bool setInputSource(llvm::StringRef inputFile, const char *text,
                      size_t textLength);

  bool setInputSource(llvm::StringRef inputFile);

  void addIncludePath(const char *path);

  void setOutputType(SlangCompilerOutputTy outputType);

  inline bool setOutput(FILE *stream) {
    if(stream == NULL)
      return false;

    mOS.reset(
        new llvm::raw_fd_ostream(fileno(stream), /* shouldClose */false));
    return true;
  }

  bool setOutput(const char *outputFile);

  inline void allowRSPrefix() {
    mAllowRSPrefix = true;
  }

  int compile();

  // The package name that's really applied will be filled in realPackageName.
  // bSize is the buffer realPackageName size.
  bool reflectToJava(const char *outputPackageName,
                     char *realPackageName, int bSize);
  bool reflectToJavaPath(const char *outputPathName);

  inline const char *getErrorMessage() {
    return mDiagClient->str().c_str();
  }

  void getPragmas(size_t *actualStringCount, size_t maxStringCount,
                  char **strings);

  const char *exportFuncs();

  // Reset the slang compiler state such that it can be reused to compile
  // another file
  inline void reset() {
    // Seems there's no way to clear the diagnostics. We just re-create it.
    createDiagnostic();
    mOutputType = SlangCompilerOutput_Default;
    return;
  }

  ~Slang();
};

}

#endif  // _SLANG_COMPILER_SLANG_HPP
