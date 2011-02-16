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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_H_

#include <cstdio>
#include <string>
#include <vector>

#include "clang/Basic/TargetOptions.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringRef.h"

#include "slang_diagnostic_buffer.h"
#include "slang_pragma_recorder.h"

namespace llvm {
  class tool_output_file;
}

namespace clang {
  class Diagnostic;
  class FileManager;
  class FileSystemOptions;
  class SourceManager;
  class LangOptions;
  class Preprocessor;
  class TargetOptions;
  class CodeGenOptions;
  class ASTContext;
  class ASTConsumer;
  class Backend;
  class TargetInfo;
}

namespace slang {

class Slang {
  static clang::LangOptions LangOpts;
  static clang::CodeGenOptions CodeGenOpts;

  static bool GlobalInitialized;

  static void LLVMErrorHandler(void *UserData, const std::string &Message);

 public:
  typedef enum {
    OT_Dependency,
    OT_Assembly,
    OT_LLVMAssembly,
    OT_Bitcode,
    OT_Nothing,
    OT_Object,

    OT_Default = OT_Bitcode
  } OutputType;

 private:
  bool mInitialized;

  // The diagnostics engine instance (for status reporting during compilation)
  llvm::IntrusiveRefCntPtr<clang::Diagnostic> mDiagnostics;
  // The diagnostics id
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> mDiagIDs;
  // The clients of diagnostics engine. The ownership is taken by the
  // mDiagnostics after creation.
  DiagnosticBuffer *mDiagClient;
  void createDiagnostic();

  // The target being compiled for
  clang::TargetOptions mTargetOpts;
  llvm::OwningPtr<clang::TargetInfo> mTarget;
  void createTarget(const std::string &Triple, const std::string &CPU,
                    const std::vector<std::string> &Features);

  // Below is for parsing and code generation

  // The file manager (for prepocessor doing the job such as header file search)
  llvm::OwningPtr<clang::FileManager> mFileMgr;
  llvm::OwningPtr<clang::FileSystemOptions> mFileSysOpt;
  void createFileManager();

  // The source manager (responsible for the source code handling)
  llvm::OwningPtr<clang::SourceManager> mSourceMgr;
  void createSourceManager();

  // The preprocessor (source code preprocessor)
  llvm::OwningPtr<clang::Preprocessor> mPP;
  void createPreprocessor();

  // The AST context (the context to hold long-lived AST nodes)
  llvm::OwningPtr<clang::ASTContext> mASTContext;
  void createASTContext();

  // The AST consumer, responsible for code generation
  llvm::OwningPtr<clang::ASTConsumer> mBackend;

  // Input file name
  std::string mInputFileName;
  std::string mOutputFileName;

  std::string mDepOutputFileName;
  std::string mDepTargetBCFileName;
  std::vector<std::string> mAdditionalDepTargets;
  std::vector<std::string> mGeneratedFileNames;

  OutputType mOT;

  // Output stream
  llvm::OwningPtr<llvm::tool_output_file> mOS;
  // Dependency output stream
  llvm::OwningPtr<llvm::tool_output_file> mDOS;

  std::vector<std::string> mIncludePaths;

 protected:
  PragmaList mPragmas;

  inline clang::Diagnostic &getDiagnostics() { return *mDiagnostics; }
  inline const clang::TargetInfo &getTargetInfo() const { return *mTarget; }
  inline clang::FileManager &getFileManager() { return *mFileMgr; }
  inline clang::SourceManager &getSourceManager() { return *mSourceMgr; }
  inline clang::Preprocessor &getPreprocessor() { return *mPP; }
  inline clang::ASTContext &getASTContext() { return *mASTContext; }

  inline const clang::TargetOptions &getTargetOptions() const
    { return mTargetOpts; }

  virtual void initDiagnostic() {}
  virtual void initPreprocessor() {}
  virtual void initASTContext() {}

  virtual clang::ASTConsumer
  *createBackend(const clang::CodeGenOptions& CodeGenOpts,
                 llvm::raw_ostream *OS,
                 OutputType OT);

 public:
  static const llvm::StringRef PragmaMetadataName;

  static void GlobalInitialization();

  Slang();

  void init(const std::string &Triple, const std::string &CPU,
            const std::vector<std::string> &Features);

  bool setInputSource(llvm::StringRef InputFile, const char *Text,
                      size_t TextLength);

  bool setInputSource(llvm::StringRef InputFile);

  inline const std::string &getInputFileName() const { return mInputFileName; }

  inline void setIncludePaths(const std::vector<std::string> &IncludePaths) {
    mIncludePaths = IncludePaths;
  }

  inline void setOutputType(OutputType OT) { mOT = OT; }

  bool setOutput(const char *OutputFile);
  inline const std::string &getOutputFileName() const {
    return mOutputFileName;
  }

  bool setDepOutput(const char *OutputFile);
  inline void setDepTargetBC(const char *TargetBCFile) {
    mDepTargetBCFileName = TargetBCFile;
  }
  inline void setAdditionalDepTargets(
      const std::vector<std::string> &AdditionalDepTargets) {
    mAdditionalDepTargets = AdditionalDepTargets;
  }
  inline void appendGeneratedFileName(
      const std::string &GeneratedFileName) {
    mGeneratedFileNames.push_back(GeneratedFileName);
  }

  int generateDepFile();
  int compile();

  inline const char *getErrorMessage() { return mDiagClient->str().c_str(); }

  // Reset the slang compiler state such that it can be reused to compile
  // another file
  virtual void reset();

  virtual ~Slang();
};

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_H_  NOLINT
