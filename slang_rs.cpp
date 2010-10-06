#include "slang_rs.h"

#include "clang/Sema/SemaDiagnostic.h"

#include "slang_rs_backend.h"
#include "slang_rs_context.h"

using namespace slang;

void SlangRS::initDiagnostic() {
  clang::Diagnostic &Diag = getDiagnostics();
  if (!Diag.setDiagnosticGroupMapping(
        "implicit-function-declaration", clang::diag::MAP_ERROR))
    assert(false && "Unable to find option group "
        "implicit-function-declaration");
  Diag.setDiagnosticMapping(
      clang::diag::ext_typecheck_convert_discards_qualifiers,
      clang::diag::MAP_ERROR);
  return;
}

void SlangRS::initPreprocessor() {
  clang::Preprocessor &PP = getPreprocessor();

  std::string inclFiles("#include \"rs_types.rsh\"");
  PP.setPredefines(inclFiles + "\n" + "#include \"rs_math.rsh\"" + "\n");
  return;
}

void SlangRS::initASTContext() {
  mRSContext = new RSContext(&getPreprocessor(),
                             &getASTContext(),
                             &getTargetInfo());
  return;
}

clang::ASTConsumer
*SlangRS::createBackend(const clang::CodeGenOptions& CodeGenOpts,
                        llvm::raw_ostream *OS,
                        Slang::OutputType OT) {
    return new RSBackend(mRSContext,
                         getDiagnostics(),
                         CodeGenOpts,
                         getTargetOptions(),
                         mPragmas,
                         OS,
                         OT,
                         getSourceManager(),
                         mAllowRSPrefix);
}

SlangRS::SlangRS(const char *Triple, const char *CPU, const char **Features)
    : Slang(Triple, CPU, Features),  mRSContext(NULL), mAllowRSPrefix(false) {
  return;
}

bool SlangRS::reflectToJava(const char *OutputPackageName,
                            char *RealPackageNameBuf,
                            int BufSize) {
  if (mRSContext)
    return mRSContext->reflectToJava(OutputPackageName,
                                     getInputFileName(),
                                     getOutputFileName(),
                                     RealPackageNameBuf,
                                     BufSize);
  else
    return false;
}

bool SlangRS::reflectToJavaPath(const char *OutputPathName) {
  if (mRSContext)
    return mRSContext->reflectToJavaPath(OutputPathName);
  else
    return false;
}

SlangRS::~SlangRS() {
  delete mRSContext;
  return;
}
