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
  static bool IsRSHeaderFile(const char *File);

  SlangRS(const std::string &Triple, const std::string &CPU,
          const std::vector<std::string> &Features);

  // The package name that's really applied will be filled in RealPackageName.
  bool reflectToJava(const std::string &OutputPathBase,
                     const std::string &OutputPackageName,
                     std::string *RealPackageName);

  virtual void reset();

  inline void allowRSPrefix(bool V = true) { mAllowRSPrefix = V; }

  virtual ~SlangRS();
};
}

#endif  // _SLANG_COMPILER_SLANG_RS_HPP
