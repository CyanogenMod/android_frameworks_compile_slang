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

#include "slang_rs.h"

#include <cstring>

#include "clang/Frontend/FrontendDiagnostic.h"

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

bool SlangRS::reflectToJava(const std::string &OutputPathBase,
                            const std::string &OutputPackageName,
                            std::string *RealPackageName) {
  return mRSContext->reflectToJava(OutputPathBase,
                                   OutputPackageName,
                                   getInputFileName(),
                                   getOutputFileName(),
                                   RealPackageName);
}

bool SlangRS::generateBitcodeAccessor(const std::string &OutputPathBase,
                                      const std::string &PackageName) {
  RSSlangReflectUtils::BitCodeAccessorContext BCAccessorContext;

  BCAccessorContext.rsFileName = getInputFileName().c_str();
  BCAccessorContext.bcFileName = getOutputFileName().c_str();
  BCAccessorContext.reflectPath = OutputPathBase.c_str();
  BCAccessorContext.packageName = PackageName.c_str();
  BCAccessorContext.bcStorage = BCST_JAVA_CODE;   // Must be BCST_JAVA_CODE

  return RSSlangReflectUtils::GenerateBitCodeAccessor(BCAccessorContext);
}


void SlangRS::initDiagnostic() {
  clang::Diagnostic &Diag = getDiagnostics();
  if (Diag.setDiagnosticGroupMapping("implicit-function-declaration",
                                     clang::diag::MAP_ERROR))
    Diag.Report(clang::diag::warn_unknown_warning_option)
        << "implicit-function-declaration";

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

bool SlangRS::compile(
    const std::list<std::pair<const char*, const char*> > &IOFiles,
    const std::list<std::pair<const char*, const char*> > &DepFiles,
    const std::vector<std::string> &IncludePaths,
    const std::vector<std::string> &AdditionalDepTargets,
    Slang::OutputType OutputType, BitCodeStorageType BitcodeStorage,
    bool AllowRSPrefix, bool OutputDep,
    const std::string &JavaReflectionPathBase,
    const std::string &JavaReflectionPackageName) {
  if (IOFiles.empty())
    return true;

  if (OutputDep && (DepFiles.size() != IOFiles.size())) {
    fprintf(stderr, "SlangRS::compile() : Invalid parameter for output "
                    "dependencies files.\n");
    return false;
  }

  std::string RealPackageName;

  const char *InputFile, *OutputFile, *BCOutputFile, *DepOutputFile;
  std::list<std::pair<const char*, const char*> >::const_iterator
      IOFileIter = IOFiles.begin(), DepFileIter = DepFiles.begin();

  setIncludePaths(IncludePaths);
  setOutputType(OutputType);
  if (OutputDep)
    setAdditionalDepTargets(AdditionalDepTargets);

  mAllowRSPrefix = AllowRSPrefix;

  for (unsigned i = 0, e = IOFiles.size(); i != e; i++) {
    InputFile = IOFileIter->first;
    OutputFile = IOFileIter->second;

    reset();

    if (!setInputSource(InputFile))
      return false;

    if (!setOutput(OutputFile))
      return false;

    if (OutputDep) {
      BCOutputFile = DepFileIter->first;
      DepOutputFile = DepFileIter->second;

      setDepTargetBC(BCOutputFile);

      if (!setDepOutput(DepOutputFile))
        return false;

      if (generateDepFile() > 0)
        return false;

      DepFileIter++;
    }

    if (Slang::compile() > 0)
      return false;

    if (OutputType != Slang::OT_Dependency) {
      if (!reflectToJava(JavaReflectionPathBase,
                         JavaReflectionPackageName,
                         &RealPackageName))
        return false;

      if ((OutputType == Slang::OT_Bitcode) &&
          (BitcodeStorage == BCST_JAVA_CODE) &&
          !generateBitcodeAccessor(JavaReflectionPathBase,
                                     RealPackageName.c_str()))
          return false;
    }

    IOFileIter++;
  }

  return true;
}

SlangRS::~SlangRS() {
  delete mRSContext;
  return;
}
