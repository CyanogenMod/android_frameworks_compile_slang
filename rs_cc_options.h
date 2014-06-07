/*
 * Copyright 2014, The Android Open Source Project
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

#ifndef _FRAMEWORKS_COMPILE_SLANG_RS_CC_OPTIONS_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_RS_CC_OPTIONS_H_

#include "llvm/Option/ArgList.h"
#include "llvm/Option/Option.h"
#include "llvm/Option/OptTable.h"

#include "slang.h"
#include "slang_rs_reflect_utils.h"

#include <string>
#include <vector>

namespace llvm {
namespace opt {
class OptTable;
}
}

namespace slang {

// Options for the RenderScript compiler llvm-rs-cc
class RSCCOptions {
 public:
  // The include search paths
  std::vector<std::string> mIncludePaths;

  // The output directory, if any.
  std::string mOutputDir;

  // The output type
  slang::Slang::OutputType mOutputType;

  unsigned mAllowRSPrefix : 1;

  // The name of the target triple to compile for.
  std::string mTriple;

  // The name of the target CPU to generate code for.
  std::string mCPU;

  // The list of target specific features to enable or disable -- this should
  // be a list of strings starting with by '+' or '-'.
  std::vector<std::string> mFeatures;

  std::string mJavaReflectionPathBase;

  std::string mJavaReflectionPackageName;

  std::string mRSPackageName;

  slang::BitCodeStorageType mBitcodeStorage;

  unsigned mOutputDep : 1;

  std::string mOutputDepDir;

  std::vector<std::string> mAdditionalDepTargets;

  unsigned mShowHelp : 1;     // Show the -help text.
  unsigned mShowVersion : 1;  // Show the -version text.

  unsigned int mTargetAPI;

  // Enable emission of debugging symbols
  unsigned mDebugEmission : 1;

  // The optimization level used in CodeGen, and encoded in emitted bitcode
  llvm::CodeGenOpt::Level mOptimizationLevel;

  RSCCOptions() {
    mOutputType = slang::Slang::OT_Bitcode;
    // Triple/CPU/Features must be hard-coded to our chosen portable ABI.
    mTriple = "armv7-none-linux-gnueabi";
    mCPU = "";
    mFeatures.push_back("+long64");
    mBitcodeStorage = slang::BCST_APK_RESOURCE;
    mOutputDep = 0;
    mShowHelp = 0;
    mShowVersion = 0;
    mTargetAPI = RS_VERSION;
    mDebugEmission = 0;
    mOptimizationLevel = llvm::CodeGenOpt::Aggressive;
  }
};

/* Return a valid OptTable (useful for dumping help information)
 */
llvm::opt::OptTable *createRSCCOptTable();

/* Parse ArgVector and return a list of Inputs (source files) and Opts
 * (options affecting the compilation of those source files).
 *
 * \param ArgVector - the input arguments to llvm-rs-cc
 * \param Inputs - returned list of actual input source filenames
 * \param Opts - returned options after command line has been processed
 * \param DiagEngine - input for issuing warnings/errors on arguments
 */
void ParseArguments(llvm::SmallVectorImpl<const char *> &ArgVector,
                    llvm::SmallVectorImpl<const char *> &Inputs,
                    RSCCOptions &Opts, clang::DiagnosticsEngine &DiagEngine);

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_RS_CC_OPTIONS_H_
