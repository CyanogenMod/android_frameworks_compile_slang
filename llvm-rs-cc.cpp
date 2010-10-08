#include "slang.h"

#include <set>
#include <string>
#include <cstdlib>

#include "llvm/ADT/SmallVector.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"

#include "llvm/System/Path.h"

#include "clang/Driver/Arg.h"
#include "clang/Driver/ArgList.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Option.h"
#include "clang/Driver/OptTable.h"

#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "slang_rs.h"
#include "slang_rs_reflect_utils.h"

using namespace slang;

using namespace clang::driver::options;

// Class under clang::driver used are enumerated here.
using clang::driver::Arg;
using clang::driver::ArgList;
using clang::driver::InputArgList;
using clang::driver::Option;
using clang::driver::OptTable;
using clang::driver::arg_iterator;

// SaveStringInSet, ExpandArgsFromBuf and ExpandArgv are all copied from
// $(CLANG_ROOT)/tools/driver/driver.cpp for processing argc/argv passed in
// main().
static inline const char *SaveStringInSet(std::set<std::string> &SavedStrings,
                                          llvm::StringRef S) {
  return SavedStrings.insert(S).first->c_str();
}
static void ExpandArgsFromBuf(const char *Arg,
                              llvm::SmallVectorImpl<const char*> &ArgVector,
                              std::set<std::string> &SavedStrings);
static void ExpandArgv(int argc, const char **argv,
                       llvm::SmallVectorImpl<const char*> &ArgVector,
                       std::set<std::string> &SavedStrings);

enum {
  OPT_INVALID = 0,  // This is not an option ID.
#define OPTION(NAME, ID, KIND, GROUP, ALIAS, FLAGS, PARAM, \
               HELPTEXT, METAVAR) OPT_##ID,
#include "RSCCOptions.inc"
  LastOption
#undef OPTION
};

static const OptTable::Info RSCCInfoTable[] = {
#define OPTION(NAME, ID, KIND, GROUP, ALIAS, FLAGS, PARAM, \
               HELPTEXT, METAVAR)   \
  { NAME, HELPTEXT, METAVAR, Option::KIND##Class, FLAGS, PARAM, \
    OPT_##GROUP, OPT_##ALIAS },
#include "RSCCOptions.inc"
};

class RSCCOptTable : public OptTable {
 public:
  RSCCOptTable()
      : OptTable(RSCCInfoTable,
                 sizeof(RSCCInfoTable) / sizeof(RSCCInfoTable[0])) {
  }
};

OptTable *createRSCCOptTable() {
  return new RSCCOptTable();
}

///////////////////////////////////////////////////////////////////////////////

class RSCCOptions {
 public:
  // The include search paths
  std::vector<std::string> mIncludePaths;

  // The output directory, if any.
  std::string mOutputDir;

  // The output type
  Slang::OutputType mOutputType;

  unsigned mAllowRSPrefix : 1;

  // If given, the name of the target triple to compile for. If not given the
  // target will be selected to match the host.
  std::string mTriple;

  // If given, the name of the target CPU to generate code for.
  std::string mCPU;

  // The list of target specific features to enable or disable -- this should
  // be a list of strings starting with by '+' or '-'.
  std::vector<std::string> mFeatures;

  std::string mJavaReflectionPathBase;

  std::string mJavaReflectionPackageName;

  BitCodeStorageType mBitcodeStorage;

  unsigned mOutputDep : 1;

  std::string mOutputDepDir;

  std::vector<std::string> mAdditionalDepTargets;

  unsigned mShowHelp : 1;  // Show the -help text.
  unsigned mShowVersion : 1;  // Show the -version text.

  RSCCOptions() {
    mOutputType = Slang::OT_Bitcode;
    mBitcodeStorage = BCST_APK_RESOURCE;
    mOutputDep = 0;
    mShowHelp = 0;
    mShowVersion = 0;
  }
};

// ParseArguments -
static void ParseArguments(llvm::SmallVectorImpl<const char*> &ArgVector,
                           llvm::SmallVectorImpl<const char*> &Inputs,
                           RSCCOptions &Opts,
                           clang::Diagnostic &Diags) {
  if (ArgVector.size() > 1) {
    const char **ArgBegin = ArgVector.data() + 1;
    const char **ArgEnd = ArgVector.data() + ArgVector.size();
    unsigned MissingArgIndex, MissingArgCount;
    llvm::OwningPtr<OptTable> OptParser(createRSCCOptTable());
    llvm::OwningPtr<InputArgList> Args(
      OptParser->ParseArgs(ArgBegin, ArgEnd, MissingArgIndex, MissingArgCount));

    // Check for missing argument error.
    if (MissingArgCount)
      Diags.Report(clang::diag::err_drv_missing_argument)
        << Args->getArgString(MissingArgIndex) << MissingArgCount;

    // Issue errors on unknown arguments.
    for (arg_iterator it = Args->filtered_begin(OPT_UNKNOWN),
        ie = Args->filtered_end(); it != ie; ++it)
      Diags.Report(clang::diag::err_drv_unknown_argument)
        << (*it)->getAsString(*Args);

    for (ArgList::const_iterator it = Args->begin(), ie = Args->end();
        it != ie; ++it) {
      const Arg *A = *it;
      if (A->getOption().getKind() == Option::InputClass)
        Inputs.push_back(A->getValue(*Args));
    }

    Opts.mIncludePaths = Args->getAllArgValues(OPT_I);

    Opts.mOutputDir = Args->getLastArgValue(OPT_o);

    if (const Arg *A = Args->getLastArg(OPT_M_Group)) {
      switch (A->getOption().getID()) {
        case OPT_M: {
          Opts.mOutputDep = 1;
          Opts.mOutputType = Slang::OT_Dependency;
          break;
        }
        case OPT_MD: {
          Opts.mOutputDep = 1;
          Opts.mOutputType = Slang::OT_Bitcode;
        }
        default: {
          assert(false && "Invalid option in M group!");
        }
      }
    }

    if (const Arg *A = Args->getLastArg(OPT_Output_Type_Group)) {
      switch (A->getOption().getID()) {
        case OPT_emit_asm: {
          Opts.mOutputType = Slang::OT_Assembly;
          break;
        }
        case OPT_emit_llvm: {
          Opts.mOutputType = Slang::OT_LLVMAssembly;
          break;
        }
        case OPT_emit_bc: {
          Opts.mOutputType = Slang::OT_Bitcode;
          break;
        }
        case OPT_emit_nothing: {
          Opts.mOutputType = Slang::OT_Nothing;
          break;
        }
        default: {
          assert(false && "Invalid option in output type group!");
        }
      }
    }

    if (Opts.mOutputType != Slang::OT_Bitcode)
      Diags.Report(clang::diag::err_drv_argument_not_allowed_with)
          << Args->getLastArg(OPT_M_Group)->getAsString(*Args)
          << Args->getLastArg(OPT_Output_Type_Group)->getAsString(*Args);

    Opts.mAllowRSPrefix = Args->hasArg(OPT_allow_rs_prefix);

    Opts.mTriple = Args->getLastArgValue(OPT_triple);
    Opts.mCPU = Args->getLastArgValue(OPT_target_cpu);
    Opts.mFeatures = Args->getAllArgValues(OPT_target_feature);

    Opts.mJavaReflectionPathBase =
        Args->getLastArgValue(OPT_java_reflection_path_base);
    Opts.mJavaReflectionPackageName =
        Args->getLastArgValue(OPT_java_reflection_package_name);

    llvm::StringRef BitcodeStorageValue =
        Args->getLastArgValue(OPT_bitcode_storage);
    if (BitcodeStorageValue == "ar")
      Opts.mBitcodeStorage = BCST_APK_RESOURCE;
    else if (BitcodeStorageValue == "jc")
      Opts.mBitcodeStorage = BCST_JAVA_CODE;
    else if (!BitcodeStorageValue.empty())
      Diags.Report(clang::diag::err_drv_invalid_value)
          << OptParser->getOptionName(OPT_bitcode_storage)
          << BitcodeStorageValue;

    Opts.mOutputDepDir =
        Args->getLastArgValue(OPT_output_dep_dir, Opts.mOutputDir);
    Opts.mAdditionalDepTargets =
        Args->getAllArgValues(OPT_additional_dep_target);

    Opts.mShowHelp = Args->hasArg(OPT_help);
    Opts.mShowVersion = Args->hasArg(OPT_version);
  }

  return;
}

static const char *DetermineOutputFile(const std::string &OutputDir,
                                       const char *InputFile,
                                       Slang::OutputType OutputTyupe,
                                       std::set<std::string> &SavedStrings) {
  if (OutputTyupe == Slang::OT_Nothing)
    return "/dev/null";

  std::string OutputFile(OutputDir);

  // Append '/' to Opts.mOutputDir if not presents
  if (!OutputFile.empty() &&
      (OutputFile[OutputFile.size() - 1]) != '/')
    OutputFile.append(1, '/');

  if (OutputTyupe == Slang::OT_Dependency)
    // The build system wants the .d file name stem to be exactly the same as
    // the source .rs file, instead of the .bc file.
    OutputFile.append(RSSlangReflectUtils::GetFileNameStem(InputFile));
  else
    OutputFile.append(RSSlangReflectUtils::BCFileNameFromRSFileName(InputFile));

  switch (OutputTyupe) {
    case Slang::OT_Dependency: {
      OutputFile.append(".d");
      break;
    }
    case Slang::OT_Assembly: {
      OutputFile.append(".S");
      break;
    }
    case Slang::OT_LLVMAssembly: {
      OutputFile.append(".ll");
      break;
    }
    case Slang::OT_Object: {
      OutputFile.append(".o");
      break;
    }
    case Slang::OT_Bitcode: {
      OutputFile.append(".bc");
      break;
    }
    case Slang::OT_Nothing:
    default: {
      assert(false && "Invalid output type!");
    }
  }

  return SaveStringInSet(SavedStrings, OutputFile);
}

static bool GenerateBitcodeAccessor(const char *InputFile,
                                    const char *OutputFile,
                                    const char *PackageName,
                                    const RSCCOptions &Opts) {
  if ((Opts.mOutputType != Slang::OT_Bitcode) ||
      (Opts.mBitcodeStorage != BCST_JAVA_CODE))
    return true;

  RSSlangReflectUtils::BitCodeAccessorContext BCAccessorContext;
  BCAccessorContext.rsFileName = InputFile;
  BCAccessorContext.bcFileName = OutputFile;
  BCAccessorContext.reflectPath = Opts.mJavaReflectionPathBase.c_str();
  BCAccessorContext.packageName = PackageName;
  BCAccessorContext.bcStorage = Opts.mBitcodeStorage;

  return RSSlangReflectUtils::GenerateBitCodeAccessor(BCAccessorContext);
}

// ExecuteCompilation -
static bool ExecuteCompilation(SlangRS &Compiler,
                               const char *InputFile,
                               const char *OutputFile,
                               const char *DepOutputFile,
                               const RSCCOptions &Opts) {
  std::string RealPackageName;

  Compiler.reset();

  Compiler.setIncludePaths(Opts.mIncludePaths);
  Compiler.setOutputType(Opts.mOutputType);
  Compiler.allowRSPrefix(Opts.mAllowRSPrefix);

  if (!Compiler.setInputSource(InputFile))
    return false;

  if (!Compiler.setOutput(OutputFile))
    return false;

  if (Opts.mOutputDep) {
    if (!Compiler.setDepOutput(DepOutputFile))
      return false;

    if (Opts.mOutputType != Slang::OT_Dependency)
      Compiler.setDepTargetBC(OutputFile);
    Compiler.setAdditionalDepTargets(Opts.mAdditionalDepTargets);

    if (Compiler.generateDepFile() > 0)
      return false;
  }

  if (Compiler.compile() > 0)
    return false;

  if (Opts.mOutputType != Slang::OT_Dependency) {
    if (!Compiler.reflectToJava(Opts.mJavaReflectionPathBase,
                                Opts.mJavaReflectionPackageName,
                                &RealPackageName))
      return false;

    if (!GenerateBitcodeAccessor(InputFile,
                                 OutputFile,
                                 RealPackageName.c_str(),
                                 Opts))
      return false;
  }

  return true;
}

int main(int argc, const char **argv) {
  std::set<std::string> SavedStrings;
  llvm::SmallVector<const char*, 256> ArgVector;
  RSCCOptions Opts;
  llvm::SmallVector<const char*, 16> Inputs;
  std::string Argv0;

  atexit(llvm::llvm_shutdown);

  ExpandArgv(argc, argv, ArgVector, SavedStrings);

  // Argv0
  llvm::sys::Path Path = llvm::sys::Path(ArgVector[0]);
  Argv0 = Path.getBasename();

  // Setup diagnostic engine
  clang::TextDiagnosticPrinter *DiagClient =
    new clang::TextDiagnosticPrinter(llvm::errs(), clang::DiagnosticOptions());
  DiagClient->setPrefix(Argv0);
  clang::Diagnostic Diags(DiagClient);

  Slang::GlobalInitialization();

  ParseArguments(ArgVector, Inputs, Opts, Diags);

  // Exits when there's any error occurred during parsing the arguments
  if (Diags.getNumErrors() > 0)
    return 1;

  if (Opts.mShowHelp) {
    llvm::OwningPtr<OptTable> OptTbl(createRSCCOptTable());
    OptTbl->PrintHelp(llvm::outs(), Argv0.c_str(),
                      "RenderScript source compiler");
    return 0;
  }

  if (Opts.mShowVersion) {
    llvm::cl::PrintVersionMessage();
    return 0;
  }

  // No input file
  if (Inputs.empty()) {
    Diags.Report(clang::diag::err_drv_no_input_files);
    return 1;
  }

  const char *InputFile, *OutputFile, *DepOutputFile = NULL;
  llvm::OwningPtr<SlangRS> Compiler(new SlangRS(Opts.mTriple, Opts.mCPU,
                                                Opts.mFeatures));

  for (int i = 0, e = Inputs.size(); i != e; i++) {
    InputFile = Inputs[i];
    OutputFile = DetermineOutputFile(Opts.mOutputDir, InputFile,
                                     Opts.mOutputType, SavedStrings);
    if (Opts.mOutputDep) {
      if (Opts.mOutputType == Slang::OT_Dependency)
        DepOutputFile = OutputFile;
      else
        DepOutputFile = DetermineOutputFile(Opts.mOutputDepDir, InputFile,
                                            Slang::OT_Dependency, SavedStrings);
    }
    if (!ExecuteCompilation(*Compiler,
                            InputFile,
                            OutputFile,
                            DepOutputFile,
                            Opts)) {
      llvm::errs() << "Failed to compile '" << InputFile << "' ("
                   << Compiler->getErrorMessage() << ")";
      break;
    }
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

// ExpandArgsFromBuf -
static void ExpandArgsFromBuf(const char *Arg,
                              llvm::SmallVectorImpl<const char*> &ArgVector,
                              std::set<std::string> &SavedStrings) {
  const char *FName = Arg + 1;
  llvm::MemoryBuffer *MemBuf = llvm::MemoryBuffer::getFile(FName);
  if (!MemBuf) {
    ArgVector.push_back(SaveStringInSet(SavedStrings, Arg));
    return;
  }

  const char *Buf = MemBuf->getBufferStart();
  char InQuote = ' ';
  std::string CurArg;

  for (const char *P = Buf; ; ++P) {
    if (*P == '\0' || (isspace(*P) && InQuote == ' ')) {
      if (!CurArg.empty()) {
        if (CurArg[0] != '@') {
          ArgVector.push_back(SaveStringInSet(SavedStrings, CurArg));
        } else {
          ExpandArgsFromBuf(CurArg.c_str(), ArgVector, SavedStrings);
        }

        CurArg = "";
      }
      if (*P == '\0')
        break;
      else
        continue;
    }

    if (isspace(*P)) {
      if (InQuote != ' ')
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '"' || *P == '\'') {
      if (InQuote == *P)
        InQuote = ' ';
      else if (InQuote == ' ')
        InQuote = *P;
      else
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '\\') {
      ++P;
      if (*P != '\0')
        CurArg.push_back(*P);
      continue;
    }
    CurArg.push_back(*P);
  }
  delete MemBuf;
}

// ExpandArgsFromBuf -
static void ExpandArgv(int argc, const char **argv,
                       llvm::SmallVectorImpl<const char*> &ArgVector,
                       std::set<std::string> &SavedStrings) {
  for (int i = 0; i < argc; ++i) {
    const char *Arg = argv[i];
    if (Arg[0] != '@') {
      ArgVector.push_back(SaveStringInSet(SavedStrings, std::string(Arg)));
      continue;
    }

    ExpandArgsFromBuf(Arg, ArgVector, SavedStrings);
  }
}
