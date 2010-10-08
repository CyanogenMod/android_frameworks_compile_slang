#include "slang_rs.h"

#include <cstring>

#include "clang/Sema/SemaDiagnostic.h"

#include "slang_rs_backend.h"
#include "slang_rs_context.h"

using namespace slang;

#define RS_HEADER_SUFFIX  "rsh"

#define ENUM_RS_HEADER()  \
  RS_HEADER_ENTRY(rs_types) \
  RS_HEADER_ENTRY(rs_cl) \
  RS_HEADER_ENTRY(rs_core) \
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

bool SlangRS::IsRSHeaderFile(const char *File) {
#define RS_HEADER_ENTRY(x)  \
  if (::strcmp(File, #x "."RS_HEADER_SUFFIX) == 0)  \
    return true;
ENUM_RS_HEADER()
#undef RS_HEADER_ENTRY
  // Deal with rs_graphics.rsh special case
  if (::strcmp(File, "rs_graphics."RS_HEADER_SUFFIX) == 0)
    return true;
  return false;
}

SlangRS::SlangRS(const std::string &Triple, const std::string &CPU,
                 const std::vector<std::string> &Features)
    : Slang(Triple, CPU, Features),
      mRSContext(NULL),
      mAllowRSPrefix(false) {
  return;
}

bool SlangRS::reflectToJava(const std::string &OutputPathBase,
                            const std::string &OutputPackageName,
                            std::string *RealPackageName) {
  if (mRSContext)
    return mRSContext->reflectToJava(OutputPathBase,
                                     OutputPackageName,
                                     getInputFileName(),
                                     getOutputFileName(),
                                     RealPackageName);
  else
    return false;
}

void SlangRS::reset() {
  Slang::reset();
  delete mRSContext;
  mRSContext = NULL;
  return;
}

SlangRS::~SlangRS() {
  delete mRSContext;
  return;
}
