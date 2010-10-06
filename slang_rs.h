#ifndef _SLANG_COMPILER_SLANG_RS_HPP
#define _SLANG_COMPILER_SLANG_RS_HPP

#include "slang.h"

namespace slang {
  class RSContext;

class SlangRS : public Slang {
 private:
  // Context for RenderScript
  RSContext *mRSContext;

  bool mAllowRSPrefix;

 protected:
  virtual void initDiagnostic();
  virtual void initPreprocessor();
  virtual void initASTContext();

  virtual clang::ASTConsumer
  *createBackend(const clang::CodeGenOptions& CodeGenOpts,
                 llvm::raw_ostream *OS,
                 Slang::OutputType OT);


 public:
  SlangRS(const char *Triple, const char *CPU, const char **Features);

  // The package name that's really applied will be filled in
  // RealPackageNameBuf. BufSize is the size of buffer RealPackageNameBuf.
  bool reflectToJava(const char *outputPackageName,
                     char *RealPackageNameBuf, int BufSize);
  bool reflectToJavaPath(const char *OutputPathName);

  inline void allowRSPrefix(bool V = true) { mAllowRSPrefix = V; }

  virtual ~SlangRS();
};
}

#endif  // _SLANG_COMPILER_SLANG_RS_HPP
