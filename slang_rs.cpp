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
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "clang/Frontend/FrontendDiagnostic.h"

#include "clang/Sema/SemaDiagnostic.h"

#include "llvm/System/Path.h"

#include "slang_rs_backend.h"
#include "slang_rs_context.h"
#include "slang_rs_export_type.h"

namespace slang {

#define RS_HEADER_SUFFIX  "rsh"

/* RS_HEADER_ENTRY(name, default_included) */
#define ENUM_RS_HEADER()  \
  RS_HEADER_ENTRY(rs_types,     1) \
  RS_HEADER_ENTRY(rs_cl,        1) \
  RS_HEADER_ENTRY(rs_core,      1) \
  RS_HEADER_ENTRY(rs_math,      1) \
  RS_HEADER_ENTRY(rs_time,      1) \
  RS_HEADER_ENTRY(rs_graphics,  0)

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

bool SlangRS::checkODR(const char *CurInputFile) {
  for (RSContext::ExportableList::iterator I = mRSContext->exportable_begin(),
          E = mRSContext->exportable_end();
       I != E;
       I++) {
    RSExportable *RSE = *I;
    if (RSE->getKind() != RSExportable::EX_TYPE)
      continue;

    RSExportType *ET = static_cast<RSExportType *>(RSE);
    if (ET->getClass() != RSExportType::ExportClassRecord)
      continue;

    RSExportRecordType *ERT = static_cast<RSExportRecordType *>(ET);

    // Artificial record types (create by us not by user in the source) always
    // conforms the ODR.
    if (ERT->isArtificial())
      continue;

    // Key to lookup ERT in ReflectedDefinitions
    llvm::StringRef RDKey(ERT->getName());
    ReflectedDefinitionListTy::const_iterator RD =
        ReflectedDefinitions.find(RDKey);

    if (RD != ReflectedDefinitions.end()) {
      const RSExportRecordType *Reflected = RD->getValue().first;
      // There's a record (struct) with the same name reflected before. Enforce
      // ODR checking - the Reflected must hold *exactly* the same "definition"
      // as the one defined previously. We say two record types A and B have the
      // same definition iff:
      //
      //  struct A {              struct B {
      //    Type(a1) a1,            Type(b1) b1,
      //    Type(a2) a2,            Type(b1) b2,
      //    ...                     ...
      //    Type(aN) aN             Type(b3) b3,
      //  };                      }
      //  Cond. #1. They have same number of fields, i.e., N = M;
      //  Cond. #2. for (i := 1 to N)
      //              Type(ai) = Type(bi) must hold;
      //  Cond. #3. for (i := 1 to N)
      //              Name(ai) = Name(bi) must hold;
      //
      // where,
      //  Type(F) = the type of field F and
      //  Name(F) = the field name.

      bool PassODR = false;
      // Cond. #1 and Cond. #2
      if (Reflected->equals(ERT)) {
        // Cond #3.
        RSExportRecordType::const_field_iterator AI = Reflected->fields_begin(),
                                                 BI = ERT->fields_begin();

        for (unsigned i = 0, e = Reflected->getFields().size(); i != e; i++) {
          if ((*AI)->getName() != (*BI)->getName())
            break;
          AI++;
          BI++;
        }
        PassODR = (AI == (Reflected->fields_end()));
      }

      if (!PassODR) {
        getDiagnostics().Report(mDiagErrorODR) << Reflected->getName()
                                               << getInputFileName()
                                               << RD->getValue().second;
        return false;
      }
    } else {
      llvm::StringMapEntry<ReflectedDefinitionTy> *ME =
          llvm::StringMapEntry<ReflectedDefinitionTy>::Create(RDKey.begin(),
                                                              RDKey.end());
      ME->setValue(std::make_pair(ERT, CurInputFile));

      if (!ReflectedDefinitions.insert(ME))
        delete ME;

      // Take the ownership of ERT such that it won't be freed in ~RSContext().
      ERT->keep();
    }
  }
  return true;
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

  mDiagErrorInvalidOutputDepParameter =
      Diag.getCustomDiagID(clang::Diagnostic::Error,
                           "invalid parameter for output dependencies files.");

  mDiagErrorODR =
      Diag.getCustomDiagID(clang::Diagnostic::Error,
                           "type '%0' in different translation unit (%1 v.s. "
                           "%2) has incompatible type definition");

  return;
}

void SlangRS::initPreprocessor() {
  clang::Preprocessor &PP = getPreprocessor();

  std::string RSH;
#define RS_HEADER_ENTRY(name, default_included)  \
  if (default_included) \
    RSH.append("#include \"" #name "."RS_HEADER_SUFFIX "\"\n");
  ENUM_RS_HEADER()
#undef RS_HEADER_ENTRY
  PP.setPredefines(RSH);

  return;
}

void SlangRS::initASTContext() {
  mRSContext = new RSContext(getPreprocessor(),
                             getASTContext(),
                             getTargetInfo(),
                             &mPragmas,
                             &mGeneratedFileNames);
  return;
}

clang::ASTConsumer
*SlangRS::createBackend(const clang::CodeGenOptions& CodeGenOpts,
                        llvm::raw_ostream *OS,
                        Slang::OutputType OT) {
    return new RSBackend(mRSContext,
                         &getDiagnostics(),
                         CodeGenOpts,
                         getTargetOptions(),
                         &mPragmas,
                         OS,
                         OT,
                         getSourceManager(),
                         mAllowRSPrefix);
}

bool SlangRS::IsRSHeaderFile(const char *File) {
#define RS_HEADER_ENTRY(name, default_included)  \
  if (::strcmp(File, #name "."RS_HEADER_SUFFIX) == 0)  \
    return true;
ENUM_RS_HEADER()
#undef RS_HEADER_ENTRY
  return false;
}

bool SlangRS::IsFunctionInRSHeaderFile(const clang::FunctionDecl *FD,
                                       const clang::SourceManager &SourceMgr) {
  clang::FullSourceLoc FSL(FD->getLocStart(), SourceMgr);
  clang::PresumedLoc PLoc = SourceMgr.getPresumedLoc(FSL);
  llvm::sys::Path HeaderFilename(PLoc.getFilename());

  return IsRSHeaderFile(HeaderFilename.getLast().data());
}

SlangRS::SlangRS() : Slang(), mRSContext(NULL), mAllowRSPrefix(false) {
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
    getDiagnostics().Report(mDiagErrorInvalidOutputDepParameter);
    return false;
  }

  std::string RealPackageName;

  const char *InputFile, *OutputFile, *BCOutputFile, *DepOutputFile;
  std::list<std::pair<const char*, const char*> >::const_iterator
      IOFileIter = IOFiles.begin(), DepFileIter = DepFiles.begin();

  setIncludePaths(IncludePaths);
  setOutputType(OutputType);
  if (OutputDep) {
    setAdditionalDepTargets(AdditionalDepTargets);
  }

  mAllowRSPrefix = AllowRSPrefix;

  for (unsigned i = 0, e = IOFiles.size(); i != e; i++) {
    InputFile = IOFileIter->first;
    OutputFile = IOFileIter->second;

    reset();

    if (!setInputSource(InputFile))
      return false;

    if (!setOutput(OutputFile))
      return false;

    if (Slang::compile() > 0)
      return false;

    if (OutputType != Slang::OT_Dependency) {
      if (!reflectToJava(JavaReflectionPathBase,
                         JavaReflectionPackageName,
                         &RealPackageName))
        return false;

      for (std::vector<std::string>::const_iterator
               I = mGeneratedFileNames.begin(), E = mGeneratedFileNames.end();
           I != E;
           I++) {
        std::string ReflectedName = RSSlangReflectUtils::ComputePackagedPath(
            JavaReflectionPathBase.c_str(),
            (RealPackageName + "/" + *I).c_str());
        appendGeneratedFileName(ReflectedName + ".java");
      }

      if ((OutputType == Slang::OT_Bitcode) &&
          (BitcodeStorage == BCST_JAVA_CODE) &&
          !generateBitcodeAccessor(JavaReflectionPathBase,
                                     RealPackageName.c_str()))
          return false;
    }

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

    if (!checkODR(InputFile))
      return false;

    IOFileIter++;
  }

  return true;
}

void SlangRS::reset() {
  delete mRSContext;
  mRSContext = NULL;
  mGeneratedFileNames.clear();
  Slang::reset();
  return;
}

SlangRS::~SlangRS() {
  delete mRSContext;
  for (ReflectedDefinitionListTy::iterator I = ReflectedDefinitions.begin(),
          E = ReflectedDefinitions.end();
       I != E;
       I++) {
    delete I->getValue().first;
  }
  return;
}

}  // namespace slang
