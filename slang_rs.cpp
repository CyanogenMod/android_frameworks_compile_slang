#include "slang_rs.h"

#include "clang/Sema/SemaDiagnostic.h"

#include "slang_rs_backend.h"
#include "slang_rs_context.h"

using namespace slang;

#define RS_HEADER_SUFFIX  "rsh"

#define ENUM_RS_HEADER()  \
  RS_HEADER_ENTRY(rs_types) \
  RS_HEADER_ENTRY(rs_cl) \
  RS_HEADER_ENTRY(rs_core) \
  RS_HEADER_ENTRY(rs_graphics) \
  RS_HEADER_ENTRY(rs_math)

#define RS_HEADER_ENTRY(x)  \
  extern const char x ## _header[]; \
  extern unsigned x ## _header_size;
ENUM_RS_HEADER()
#undef RS_HEADER_ENTRY

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

  std::string RSH;
#define RS_HEADER_ENTRY(x)  \
  RSH.append("#line 1 \"" #x "."RS_HEADER_SUFFIX"\"\n"); \
  RSH.append(x ## _header, x ## _header_size);
ENUM_RS_HEADER()
#undef RS_HEADER_ENTRY
  PP.setPredefines(RSH);

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
