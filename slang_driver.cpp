#include "slang_rs.h"
#include "slang_rs_reflect_utils.h"

#include <assert.h>
#include <getopt.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <vector>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

using namespace std;
using slang::Slang;
using slang::SlangRS;

#define ERR_NO_INPUT_FILE           "no input file"

#define NOTE(x) cerr << "note: " NOTE_ ## x << endl
#define WARN(x) cerr << "warning: " WARN_ ## x << endl
#define WARN1(x, v1) cerr << "warning: " WARN_ ## x(v1) << endl
#define WARN2(x, v1, v2) cerr << "warning: " WARN_ ## x(v1, v2) << endl

#define NOTE_MULTIPLE_INPUT_FILES   "multiple input files detected: Will enforce ODR (One Definition Rule) during reflection."

#define WARN_UNKNOWN_CPU(v1)            "the given CPU " << (v1) << " cannot be recognized, but we'll force passing it to Slang compiler"

#define WARN_MISMATCH_CPU_TARGET_ARCH(v1, v2)   \
  "CPU (target: " << (v1) << ") you selected doesn't match the target of enable features you specified or the triple string you given (" << (v2) << ")"

#define WARN_MISMATCH_FEATURE_TARGET_ARCH(v1, v2)   \
  "Feature (target: " << (v1) << ") you selected doesn't match the target of CPU you specified or the triple string you given (" << (v2) << ")"

/* List of all support target, will look like "ArchARM" */
#define MK_TARGET_ARCH(target)  Arch ## target

typedef enum {
  ArchNone,
#define DEF_SUPPORT_TARGET(target, name, default_triple)  \
  MK_TARGET_ARCH(target),
# include "target.inc"

  MaxTargetArch
} TargetArchEnum;

typedef struct {
  TargetArchEnum Arch;
  const char* Name;
  const char* DefaultTriple;
} TargetArch;

static const TargetArch TargetArchTable[] = {
  { ArchNone, "none", "unknown-unknown-linux" },
#define DEF_SUPPORT_TARGET(target, name, default_triple)  \
  { MK_TARGET_ARCH(target), name, default_triple },
# include "target.inc"
};

#if defined(__arm__)
#   define HOST_ARCH                        MK_TARGET_ARCH(X86)
#elif defined(__i386__)
#   define HOST_ARCH                        MK_TARGET_ARCH(ARM)
#elif defined(__x86_64__)
#   define HOST_ARCH                        MK_TARGET_ARCH(X64)
#else
#   error "We can not find default triple string for your host machine, please define it by yourself via option '--triple' or '-t'"
#endif

#define DEFAULT_TARGET_TRIPLE_STRING        TargetArchTable[HOST_ARCH].DefaultTriple

/* Lists of all target features, will look like "{Target}FeatureNEON" */
#define MK_TARGET_FEATURE(target, id)   target ## id
typedef enum {
  FeatureNone = 0,
#define DEF_TARGET_FEATURE(target, id, key, description)    \
  MK_TARGET_FEATURE(target, id),

#define HOOK_TARGET_FIRST_FEATURE(target, id, key, description) \
  target ## FeatureStart,                                       \
  MK_TARGET_FEATURE(target, id) = target ## FeatureStart,

#   include "target.inc"

  MaxTargetFeature
} TargetFeatureEnum;

/* Feature as bits using in {Target}TargetCPU, will look like "X{Target}FeatureNEON" */
#define MK_TARGET_FEATURE_BIT(target, id)   X ## target ## id
typedef enum {
  XFeatureNone = 0,
#define DEF_TARGET_FEATURE(target, id, key, description)    \
  MK_TARGET_FEATURE_BIT(target, id) = 1 << (MK_TARGET_FEATURE(target, id) - target ## FeatureStart),
# include "target.inc"

  XMaxTargetFeature
} TargetFeatureBit;

typedef struct {
  TargetArchEnum Arch;
  TargetFeatureEnum Key;
  TargetFeatureBit Bit;
  const char* Name;
  const char* Desc;
} TargetFeature;

/* Should be 1-1 mapping with TargetFeatureEnum */
static const TargetFeature TargetFeatureTable[] = {
  { ArchNone, FeatureNone, XFeatureNone, "none", "Empty feature" },    /* FeatureNone */
#define DEF_TARGET_FEATURE(target, id, key, description)    \
  { MK_TARGET_ARCH(target), MK_TARGET_FEATURE(target, id), MK_TARGET_FEATURE_BIT(target, id), key, description },
# include "target.inc"
};

typedef struct {
  TargetArchEnum Arch;
  const char* Name;
  const char* Desc;
  unsigned int FeatureEnabled;
} TargetCPU;

/* Sorted by CPU name such that we can call bsearch() to quickly retain the CPU entry corresponding to the name */
#define E(feature)  MK_TARGET_FEATURE_BIT(ARM, feature)
static const TargetCPU TargetCPUTable[] = {
  { MK_TARGET_ARCH(ARM), "arm1020e", "Select the arm1020e processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm1020t", "Select the arm1020t processor", E(ArchV5T) },
  { MK_TARGET_ARCH(ARM), "arm1022e", "Select the arm1022e processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm10e", "Select the arm10e processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm10tdmi", "Select the arm10tdmi processor", E(ArchV5T) },
  { MK_TARGET_ARCH(ARM), "arm1136j-s", "Select the arm1136j-s processor", E(ArchV6) },
  { MK_TARGET_ARCH(ARM), "arm1136jf-s", "Select the arm1136jf-s processor", E(ArchV6) | E(FeatureVFP2) },
  { MK_TARGET_ARCH(ARM), "arm1156t2-s", "Select the arm1156t2-s processor", E(ArchV6T2) | E(FeatureThumb2) },
  { MK_TARGET_ARCH(ARM), "arm1156t2f-s", "Select the arm1156t2f-s processor", E(ArchV6T2) | E(FeatureThumb2) | E(FeatureVFP2) },
  { MK_TARGET_ARCH(ARM), "arm1176jz-s", "Select the arm1176jz-s processor", E(ArchV6) },
  { MK_TARGET_ARCH(ARM), "arm1176jzf-s", "Select the arm1176jzf-s processor", E(ArchV6) | E(FeatureVFP2) },
  { MK_TARGET_ARCH(ARM), "arm710t", "Select the arm710t processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm720t", "Select the arm720t processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm7tdmi", "Select the arm7tdmi processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm7tdmi-s", "Select the arm7tdmi-s processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm8", "Select the arm8 processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "arm810", "Select the arm810 processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "arm9", "Select the arm9 processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm920", "Select the arm920 processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm920t", "Select the arm920t processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm922t", "Select the arm922t processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm926ej-s", "Select the arm926ej-s processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm940t", "Select the arm940t processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "arm946e-s", "Select the arm946e-s processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm966e-s", "Select the arm966e-s processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm968e-s", "Select the arm968e-s processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm9e", "Select the arm9e processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "arm9tdmi", "Select the arm9tdmi processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "cortex-a8", "Select the cortex-a8 processor", E(ArchV7A) | E(FeatureThumb2) | E(FeatureNEON) },
  { MK_TARGET_ARCH(ARM), "cortex-a9", "Select the cortex-a9 processor", E(ArchV7A) | E(FeatureThumb2) | E(FeatureNEON) },
  { MK_TARGET_ARCH(ARM), "ep9312", "Select the ep9312 processor", E(ArchV4T) },
  { MK_TARGET_ARCH(ARM), "generic", "Select the generic processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "iwmmxt", "Select the iwmmxt processor", E(ArchV5TE) },
  { MK_TARGET_ARCH(ARM), "mpcore", "Select the mpcore processor", E(ArchV6) | E(FeatureVFP2) },
  { MK_TARGET_ARCH(ARM), "mpcorenovfp", "Select the mpcorenovfp processor", E(ArchV6) },
  { MK_TARGET_ARCH(ARM), "strongarm", "Select the strongarm processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "strongarm110", "Select the strongarm110 processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "strongarm1100", "Select the strongarm1100 processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "strongarm1110", "Select the strongarm1110 processor", XFeatureNone },
  { MK_TARGET_ARCH(ARM), "xscale", "Select the xscale processor", MK_TARGET_FEATURE_BIT(ARM, ArchV5TE) }
};
#undef E
static int CompareCPUName(const void* a, const void* b) {
  return strcasecmp(((TargetCPU*) a)->Name, ((TargetCPU*) b)->Name);
}
#define NUM_TARGET_CPU (sizeof(TargetCPUTable) / sizeof(TargetCPU))

static struct option* SlangOpts = NULL;

static const char* CPUString;
static const TargetCPU* CPU;
static const char* TripleString;
static TargetFeatureEnum EnableFeatureValue, DisableFeatureValue;
static Slang::OutputType OutputFileType;
static const char* JavaReflectionPackageName;

static const char* JavaReflectionPathName;
static const char* OutputPathName;

static std::vector<std::string> IncludePaths;

static std::string* InputFileNames;
static std::string* OutputFileNames;

// Where to store the bc file.
// possible values:
// ar: packed as apk resource
// jc: encoded in Java code.
static slang::BitCodeStorageType BitCodeStorage(slang::BCST_APK_RESOURCE);

static bool Verbose;
static const char* FeatureEnabledList[MaxTargetFeature + 1];
static int AllowRSPrefix = 0;
static int Externalize = 0;
static int NoLink = 1;

/* Construct the command options table used in ParseOption::getopt_long */
static void ConstructCommandOptions() {
  /* Basic slang command option */
  static struct option BasicSlangOpts[] = {
    { "allow-rs-prefix", no_argument, &AllowRSPrefix, 1 },
    { "externalize", no_argument, &Externalize, 1 },
    { "no-link", no_argument, &NoLink, 1 },

    { "emit-llvm",       no_argument, (int*) &OutputFileType, Slang::OT_LLVMAssembly },
    { "emit-bc",         no_argument, (int*) &OutputFileType, Slang::OT_Bitcode },
    { "emit-asm",        no_argument, NULL, 'S' },
    { "emit-obj",        no_argument, NULL, 'c' },
    { "emit-nothing",    no_argument, (int*) &OutputFileType, Slang::OT_Nothing },

    { "help",    no_argument, NULL, 'h' }, /* -h */
    { "verbose", no_argument, NULL, 'v' }, /* -v */

    { "output-obj-path", required_argument, NULL, 'o' }, /* -o */
    { "cpu",    required_argument, NULL, 'u' }, /* -u */
    { "triple", required_argument, NULL, 't' }, /* -t */

    { "output-java-reflection-class", required_argument, NULL, 'j'},   /* -j */
    { "output-java-reflection-path", required_argument, NULL, 'p'},   /* -p */

    { "include-path", required_argument, NULL, 'I'},  /* -I */
    { "bitcode-storage", required_argument, NULL, 's'}, /* -s */
  };

  const int NumberOfBasicOptions = sizeof(BasicSlangOpts) / sizeof(struct option);

  SlangOpts = new struct option [ NumberOfBasicOptions + MaxTargetFeature * 2 /* for --enable-feature and --disable-feature */ ];

  /* Fill SlangOpts with basic options */
  memcpy(SlangOpts, BasicSlangOpts, sizeof(BasicSlangOpts));

  int i = NumberOfBasicOptions;
  /* Add --enable-TARGET_FEATURE option into slang command option */
#define DEF_TARGET_FEATURE(target, id, key, description)        \
  SlangOpts[i].name = "enable-" key;                            \
  SlangOpts[i].has_arg = optional_argument;                     \
  SlangOpts[i].flag = (int*) &EnableFeatureValue;               \
  SlangOpts[i].val = target ## id;                              \
  i++;
#   include "target.inc"

  /* Add --disable-TARGET_FEATURE option into slang command option */
#define DEF_TARGET_FEATURE(target, id, key, description)        \
  SlangOpts[i].name = "disable-" key;                           \
  SlangOpts[i].has_arg = optional_argument;                     \
  SlangOpts[i].flag = (int*) &DisableFeatureValue;              \
  SlangOpts[i].val = target ## id;                              \
  i++;
#   include "target.inc"

  /* NULL-terminated the SlangOpts */
  memset(&SlangOpts[i], 0, sizeof(struct option));

  return;
}


static void Usage(const char* CommandName);

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
static int FileCount;

static int AddOutputFileSuffix(std::string &pathFile) {
  switch (OutputFileType) {
    case Slang::OT_Assembly:
      pathFile += ".S";
      break;
    case Slang::OT_LLVMAssembly:
      pathFile += ".ll";
      break;
    case Slang::OT_Object:
      pathFile += ".o";
      break;

    case Slang::OT_Nothing:
      return 0; //strcpy(OutputFileNames[count], "/dev/null");

    case Slang::OT_Bitcode:
    default:
      pathFile += ".bc";
      break;
  }
  return 1;
}

static bool ParseOption(int Argc, char** Argv) {
  assert(SlangOpts != NULL && "Slang command options table was not initialized!");

  /* Set default value to option */
  CPU = NULL;
  TripleString = DEFAULT_TARGET_TRIPLE_STRING;
  EnableFeatureValue = DisableFeatureValue = FeatureNone;
  OutputFileType = Slang::OT_Default;
  JavaReflectionPackageName = NULL;

  JavaReflectionPathName = NULL;
  OutputPathName = NULL;

  IncludePaths.clear();

  InputFileNames = NULL;
  OutputFileNames = NULL;

  Verbose = false;
  FeatureEnabledList[0] = NULL;

  int ch;

  unsigned int FeatureEnableBits = 0;
  unsigned int FeatureDisableBits = 0;
#define ENABLE_FEATURE(x)                       \
  FeatureEnableBits |= (x)
#define DISABLE_FEATURE(x)                      \
  FeatureDisableBits |= (x)
  TargetArchEnum ExpectedArch = ArchNone;

  /* Turn off the error message output by getopt_long */
  opterr = 0;

  while((ch = getopt_long(Argc, Argv, "Schvo:u:t:j:p:I:s:", SlangOpts, NULL)) != -1) {
    switch(ch) {
      case 'S':
        OutputFileType = Slang::OT_Assembly;
        break;

      case 'c':
        OutputFileType = Slang::OT_Object;
        break;

      case 'o':
        OutputPathName = optarg;
        break;

      case 'j':
        JavaReflectionPackageName = optarg;
        break;

      case 'p':
        JavaReflectionPathName = optarg;
        break;

      case 'I':
        IncludePaths.push_back(optarg);
        break;

      case 's':
        if (!std::strcmp(optarg, "ar")) {
            BitCodeStorage = slang::BCST_APK_RESOURCE;
        } else if (!std::strcmp(optarg, "jc")) {
            BitCodeStorage = slang::BCST_JAVA_CODE;
        } else {
            cerr << "Error: bit code storage (-s) should be 'ar' or 'jc', saw "
                 << optarg << endl;
            return false;
        }
        break;

      case 'u':
        {
          CPUString = optarg;
          const TargetCPU SearchCPU = { ArchNone, CPUString, NULL, XFeatureNone };
          CPU = (TargetCPU*) bsearch(&SearchCPU, TargetCPUTable, sizeof(TargetCPUTable) / sizeof(TargetCPU), sizeof(TargetCPU), CompareCPUName);
          if(CPU == NULL) {
            WARN1(UNKNOWN_CPU, SearchCPU.Name);
          } else {
            CPUString = CPU->Name;

            if(ExpectedArch == ArchNone)
              ExpectedArch = CPU->Arch;
            else if(ExpectedArch != CPU->Arch) {
              WARN2(MISMATCH_CPU_TARGET_ARCH, TargetArchTable[CPU->Arch].Name, TargetArchTable[ExpectedArch].Name);
              break;
            }

            /* Get CPU Feature and enable its available feature */
            FeatureEnableBits |= CPU->FeatureEnabled;
          }
        }
        break;

      case 't':
        TripleString = optarg;
        break;

      case 'h':
        Usage(Argv[0]);
        return false;
        break;

      case 'v':
        Verbose = true;
        break;

      case 0:
        {
          if(EnableFeatureValue != FeatureNone || DisableFeatureValue != FeatureNone) {
            bool IsDisable = (DisableFeatureValue != FeatureNone);
            const TargetFeature* FeatureSelected = &TargetFeatureTable[ ((IsDisable) ? DisableFeatureValue : EnableFeatureValue) ];
            assert(FeatureSelected != NULL && "Unexpected target feature! (not presented in table but parsed!?)");

            if(ExpectedArch == ArchNone)
              ExpectedArch = FeatureSelected->Arch;
            else if(FeatureSelected->Arch != ExpectedArch) {
              WARN2(MISMATCH_FEATURE_TARGET_ARCH, TargetArchTable[FeatureSelected->Arch].Name, TargetArchTable[ExpectedArch].Name);
              break;
            }

            if(optarg != NULL && atoi(optarg) == 0)
              IsDisable = !IsDisable;

            if(IsDisable)
              DISABLE_FEATURE(FeatureSelected->Bit);
            else
              ENABLE_FEATURE(FeatureSelected->Bit);
          }
        }
        break;

      default:
        cerr << "Unknown option: " << Argv[optind - 1] << endl;
        return false;
        break;
    }
  }
#undef ENABLE_FEATURE
#undef DISABLE_FEATURE

  int CurFeatureEnableListIdx = 0;
  /* Add the enable/disable feature string to */
  switch(ExpectedArch) {
    case ArchNone:
      ExpectedArch = HOST_ARCH;
      break;

#define DEF_TARGET_FEATURE(target, id, key, description)            \
      if(FeatureDisableBits & MK_TARGET_FEATURE_BIT(target, id))        \
        FeatureEnabledList[ CurFeatureEnableListIdx++ ] = "-" key;      \
      else if(FeatureEnableBits & MK_TARGET_FEATURE_BIT(target, id))    \
        FeatureEnabledList[ CurFeatureEnableListIdx++ ] = "+" key;
#define HOOK_TARGET_FIRST_FEATURE(target, id, key, description) \
      case Arch ## target:                                      \
          /* Fix target triple */                               \
          if(TripleString == DEFAULT_TARGET_TRIPLE_STRING)              \
            TripleString = TargetArchTable[MK_TARGET_ARCH(target)].DefaultTriple; \
      DEF_TARGET_FEATURE(target, id, key, description)
#define HOOK_TARGET_LAST_FEATURE(target, id, key, description)  \
      DEF_TARGET_FEATURE(target, id, key, description)                  \
          FeatureEnabledList[ CurFeatureEnableListIdx++ ] = NULL; /* null-terminator */ \
      break;
#include "target.inc"

    default:
      assert(false && "Unknown / Unsupported CPU architecture");
      break;
  }

  Argc -= optind;
  if(Argc <= 0) {
    cerr << Argv[0] << ": "ERR_NO_INPUT_FILE << endl;
    return false;
  }

  if(Argc > 1) {
    NOTE(MULTIPLE_INPUT_FILES);
  }


  FileCount = Argc;
  InputFileNames = new std::string[FileCount];
  OutputFileNames = new std::string[FileCount];
  int count;
  for (count = 0; count < FileCount; count++) {
    InputFileNames[count].assign(Argv[optind + count]);

    if ( OutputPathName && !strcmp(OutputPathName, "-") ) {
      OutputFileNames[count].assign("stdout");
      continue;
    }

    std::string _outF;
    if (OutputPathName) {
      _outF.assign(OutputPathName);
      if (_outF[_outF.length()-1] != '/') {
        _outF += "/";
      }
    }
    _outF += slang::RSSlangReflectUtils::BCFileNameFromRSFileName(
        InputFileNames[count].c_str());

    int status = AddOutputFileSuffix(_outF);
    if (status < 0) {
      return false;
    } else if (!status) {
      OutputFileNames[count].assign("/dev/null");
    } else {
      OutputFileNames[count].assign(_outF);
    }
  }

  if(Verbose) {
    for (count = 0; count < FileCount; count++) {
      cout << "Input: " << InputFileNames[count] << endl;
    }

    if(CPU != NULL)
      cout << "Use CPU: " << CPU->Name << endl;
    cout << "Use triple string: " << TripleString << endl;
    cout << "Expected architecture: " << TargetArchTable[ExpectedArch].Name << endl;

    cout << "Enable target feature: " << endl;
    for(int i=0;FeatureEnabledList[i]!=NULL;i++)
      if(*FeatureEnabledList[i] == '+')
        cout << "\t" << &FeatureEnabledList[i][1] << endl;
    cout << endl;

    cout << "Disable target feature: " << endl;
    for(int i=0;FeatureEnabledList[i]!=NULL;i++)
      if(*FeatureEnabledList[i] == '-')
        cout << "\t" << &FeatureEnabledList[i][1] << endl;
    cout << endl;

    cout << "Output to: " << ((strcmp(OutputPathName, "-")) ? OutputPathName : "(standard output)") << ", type: ";
    switch(OutputFileType) {
      case Slang::OT_Assembly: cout << "Target Assembly"; break;
      case Slang::OT_LLVMAssembly: cout << "LLVM Assembly"; break;
      case Slang::OT_Bitcode: cout << "Bitcode"; break;
      case Slang::OT_Nothing: cout << "No output (test)"; break;
      case Slang::OT_Object: cout << "Object file"; break;
      default: assert(false && "Unknown output type"); break;
    }
    cout << endl;
  }

  return true;
}

static void DestroyCommandOptions() {
  if(SlangOpts != NULL) {
    delete [] SlangOpts;
    SlangOpts = NULL;
  }
  return;
}

/*
 * E.g., replace out/host/linux-x86/bin/slang to out/host/linux-x86/bin/<fileName>
 */
static std::string replaceLastPartWithFile(std::string& cmd, const char* fileName) {
  size_t pos = cmd.rfind('/');
  if (pos == std::string::npos) {
    return std::string(fileName);
  }

  cmd.resize(pos+1);
  cmd.append(fileName);
  return cmd;
  //  std::string returnFile = cmd.substr(0, pos+1).append(fileName);  // cmd.replace(pos+1, std::string::npos, fileName);
}

#define LINK_FILE "/frameworks/compile/slang/rsScriptC_Lib.bc"

static char* linkFile() {
  char* dir = getenv("ANDROID_BUILD_TOP");
  char* dirPath;
  bool readyToLink = false;
  if (dir) {
    dirPath = new char[strlen(dir) + strlen(LINK_FILE) + 1];
    strcpy(dirPath, dir);
    strcpy(dirPath + strlen(dir), LINK_FILE);
    if (open(dirPath, O_RDONLY) >= 0) {
      readyToLink = true;
    }
  }

  if (!readyToLink) {
    /* try cwd */
    int siz = 256;
    dir = new char[siz];
    while (!getcwd(dir, siz)) {
      siz *= 2;
      dir = new char[siz];
    }
    dirPath = new char[strlen(dir) + strlen(LINK_FILE) + 1];
    strcpy(dirPath, dir);
    strcpy(dirPath + strlen(dir), LINK_FILE);
    //dirPath[strlen(dir) + LINK_FILE_LENGTH] = '\0';
    if (open(dirPath, O_RDONLY) < 0) {
      cerr << "Error: Couldn't load rs library bitcode file" << endl;
      exit(1);
    }
  }
  return dirPath;
}

#define LINK_FILE1 "/frameworks/compile/slang/rslib.bc"
#define LINK_FILE1_LENGTH 30

static char* linkFile1() {
  char* dir = getenv("ANDROID_BUILD_TOP");
  char* dirPath;
  bool readyToLink = false;
  if (dir) {
    dirPath = new char[strlen(dir) + LINK_FILE1_LENGTH];
    strcpy(dirPath, dir);
    strcpy(dirPath + strlen(dir), LINK_FILE1);
    if (open(dirPath, O_RDONLY) >= 0) {
      readyToLink = true;
    }
  }

  if (!readyToLink) {
    /* try cwd */
    int siz = 256;
    dir = new char[siz];
    while (!getcwd(dir, siz)) {
      siz *= 2;
      dir = new char[siz];
    }
    dirPath = new char[strlen(dir) + LINK_FILE1_LENGTH];
    strcpy(dirPath, dir);
    strcpy(dirPath + strlen(dir), LINK_FILE1);
    if (open(dirPath, O_RDONLY) < 0) {
      cerr << "Error: Couldn't load rs library bitcode file" << endl;
      exit(1);
    }
  }
  return dirPath;
}

static int waitForChild(pid_t pid) {
  pid_t w;
  int childExitStatus;

  do {
    w = waitpid(pid, &childExitStatus, WUNTRACED | WCONTINUED);
    if (w == -1) {
      exit(1);
    }
    if (WIFEXITED(childExitStatus)) {
      if (WEXITSTATUS(childExitStatus)) {
        cerr << "Linking error" << endl;
        exit(1);
      }
      return 0;
    } else if (WIFSIGNALED(childExitStatus)) {
      cerr << "Linking: Killed by signal " << WTERMSIG(childExitStatus) << endl;
      exit(1);
    } else if (WIFSTOPPED(childExitStatus)) {
      cerr << "Linking: Stopped by signal " << WSTOPSIG(childExitStatus) << endl;
    } else if (WIFCONTINUED(childExitStatus)) {
      cerr << "LInking: Continued" << endl;
    }
  } while (!WIFEXITED(childExitStatus) && !WIFSIGNALED(childExitStatus));
  return 0;
}

#define SLANG_CALL_AND_CHECK(expr)              \
  if(!(expr)) {                                 \
    if(slang->getErrorMessage())                \
      cerr << slang->getErrorMessage();         \
    ret = 1;                                    \
    goto on_slang_error;                        \
  }

int main(int argc, char** argv) {
  int ret = 0;
  int count;

  std::string command(argv[0]);
  //  char* command = new char[strlen(argv[0])+16]; strcpy(command, argv[0]);

  if(argc < 2) {
    cerr << argv[0] << ": "ERR_NO_INPUT_FILE << endl;
    return 1;
  }

  ConstructCommandOptions();

  if(ParseOption(argc, argv)) {
    SlangRS *slang = new SlangRS(TripleString, CPUString, FeatureEnabledList);
    if(slang == NULL) {
      goto on_slang_error;
    }

    slang->setOutputType(OutputFileType);

    for (size_t i = 0; i < IncludePaths.size(); ++i) {
        slang->addIncludePath(IncludePaths[i].c_str());
    }

    if (AllowRSPrefix)
      slang->allowRSPrefix(true);

    for (count = 0; count < FileCount; count++) {
      /* Start compilation */

      SLANG_CALL_AND_CHECK( slang->setInputSource(InputFileNames[count]) );

      std::string beforeLink;
      if (NoLink) {
        SLANG_CALL_AND_CHECK( slang->setOutput(OutputFileNames[count].c_str()) );
      } else {
        std::string stem = slang::RSSlangReflectUtils::BCFileNameFromRSFileName(
            InputFileNames[count].c_str());
        char tmpFileName[256];
        snprintf(tmpFileName, sizeof(tmpFileName), "/tmp/%s.bc.XXXXXX", stem.c_str());
        int f = mkstemp(tmpFileName);
        if (f < 0) {
            cerr << "Error: could not create temporary file " << tmpFileName << endl;
            return 1;
        }
        // we don't need the file descriptor
        close(f);

        beforeLink.assign(tmpFileName);
        SLANG_CALL_AND_CHECK( slang->setOutput(beforeLink.c_str()) );
      }

      SLANG_CALL_AND_CHECK( slang->compile() <= 0 );

      /* Output log anyway */
      if(slang->getErrorMessage())
        cout << slang->getErrorMessage();

      SLANG_CALL_AND_CHECK( slang->reflectToJavaPath(JavaReflectionPathName) );

      char realPackageName[0x100];
      SLANG_CALL_AND_CHECK( slang->reflectToJava(JavaReflectionPackageName,
                            realPackageName, sizeof(realPackageName)));

      if (NoLink) {
        goto generate_bitcode_accessor;
      }

      // llvm-rs-link
      pid_t pid;
      if ((pid = fork()) < 0) {
        cerr << "Failed before llvm-rs-link" << endl;
        exit(1);
      } else if (pid == 0) {
        std::string cmd(command);
        cmd = replaceLastPartWithFile(cmd, "llvm-rs-link");

        char* link0 = linkFile();
        char* link1 = linkFile1();
        if (Externalize) {
          char* cmdline[] = { (char*)cmd.c_str(), "-e", "-o", (char*)OutputFileNames[count].c_str(), (char*)beforeLink.c_str(), (char*)link0, /*link1,*/ NULL };
          execvp(cmd.c_str(), cmdline);
        } else {
          //cerr << cmd << " -o " << OutputFileNames[count] << " " << beforeLink << " " << link0 << endl;
          char* cmdline[] = { (char*)cmd.c_str(), "-o", (char*)OutputFileNames[count].c_str(), (char*)beforeLink.c_str(), (char*)link0, /*link1,*/ NULL };
          execvp(cmd.c_str(), cmdline);
        }
      }

      waitForChild(pid);

      /*      // opt
      if ((pid = fork()) < 0) {
        cerr << "Failed before opt" << endl;
        exit(1);
      } else if (pid == 0) {
        std::string cmd(command);
        replaceLastPartWithFile(cmd, "opt");

        const char* funcNames = slangExportFuncs(slang);
        std::string internalize("-internalize-public-api-list=init,root");
        if (funcNames) {
          internalize.append(funcNames);
        }

        //cerr << cmd << " -std-link-opts " << internalize << " " << afterLink << " -o " << OutputFileNames[count] << endl;
        execl(cmd.c_str(), cmd.c_str(), "-std-link-opts", internalize.c_str(), afterLink.c_str(), "-o", OutputFileNames[count].c_str(), NULL);
      }

      waitForChild(pid); */

    generate_bitcode_accessor:
        if ((OutputFileType == Slang::OT_Bitcode)
            && (BitCodeStorage == slang::BCST_JAVA_CODE)
            && (OutputFileNames[count] != "stdout")) {
            slang::RSSlangReflectUtils::BitCodeAccessorContext bc_context;
            bc_context.rsFileName = InputFileNames[count].c_str();
            bc_context.bcFileName = OutputFileNames[count].c_str();
            bc_context.reflectPath = JavaReflectionPathName ? JavaReflectionPathName : "";
            bc_context.packageName = realPackageName;
            bc_context.bcStorage = BitCodeStorage;
            if (!slang::RSSlangReflectUtils::GenerateBitCodeAccessor(
                bc_context)) {
                ret = 1;
            }
        }
    }
 on_slang_error:
    delete slang;
  }

  DestroyCommandOptions();

  if (ret) exit(1);
  return ret;
}

/*
 * OUTPUT_OPTION is only used inside Usage()
 */
#define OUTPUT_OPTION(short_name, long_name, desc)      \
  do {                                                  \
    if(short_name)                                               \
      cout << setw(4) << right << (short_name) << ", ";          \
    else                                                         \
      cout << "      ";                                          \
    cout << setw(17) << left << (long_name);                     \
    cout << " " << (desc) << endl;                               \
  } while(false)

static void Usage(const char* CommandName) {
  cout << "Usage: " << CommandName << " [OPTION]... " << "[INPUT FILE]" << endl;

  cout << endl;

  cout << "Basic: " << endl;

  OUTPUT_OPTION("-h", "--help", "Print this help");
  OUTPUT_OPTION("-v", "--verbose", "Be verbose");
  OUTPUT_OPTION(NULL, "--allow-rs-prefix", "Allow user-defined function names with the \"rs\" prefix");
  OUTPUT_OPTION(NULL, "--no-link", "Do not link the system bitcode libraries");
  OUTPUT_OPTION("-o", "--output-obj-path=<PATH>", "Write compilation output at this path ('-' means stdout)");
  OUTPUT_OPTION("-j", "--output-java-reflection-class=<PACKAGE NAME>", "Output reflection of exportables in the native domain into Java");
  OUTPUT_OPTION("-p", "--output-java-reflection-path=<PATH>", "Write reflection output at this path");
  OUTPUT_OPTION("-I", "--include-path=<PATH>", "Add a hearder search path");
  OUTPUT_OPTION("-s", "--bitcode-storage=<VALUE>", "Where to store the bc file. 'ar' means apk resource, 'jc' means Java code.");

  cout << endl;

  cout << "Output type:" << endl;

  OUTPUT_OPTION(NULL, "--emit-llvm", "Set output type to LLVM assembly (.ll)");
  OUTPUT_OPTION(NULL, "--emit-bc", "Set output type to Bitcode (.bc) (Default)");
  OUTPUT_OPTION("-S", "--emit-asm", "Set output type to target assmbly code (.S)");
  OUTPUT_OPTION("-c", "--emit-obj", "Set output type to target object file (.o)");
  OUTPUT_OPTION(NULL, "--emit-nothing", "Output nothing");

  cout << endl;

  cout << "Code generation option: " << endl;

  OUTPUT_OPTION("-u", "--cpu=CPU", "generate the assembly / object file for the CPU");
  cout << endl;
  cout << "\tAvailable CPU:" << endl;

  for (unsigned i = 0; i < NUM_TARGET_CPU; i++)
    cout << "\t" << setw(13) << right << TargetCPUTable[i].Name << left << ": (" << TargetArchTable[(TargetCPUTable[i].Arch)].Name << ") " << TargetCPUTable[i].Desc << endl;

  cout << endl;

  OUTPUT_OPTION("-t", "--triple=TRIPLE", "generate the assembly / object file for the Triple");
  cout << "\tDefault triple: " << endl;
#define DEF_SUPPORT_TARGET(target, name, default_triple)                \
  cout << "\t" << setw(5) << right << name << left << ": " << default_triple << endl;
#include "target.inc"
  cout << endl;

  OUTPUT_OPTION(NULL, "--enable-FEATURE", "enable the FEATURE for the generation of the assembly / object file");
  OUTPUT_OPTION(NULL, "--disable-FEATURE", "disable the FEATURE for the generation of the assembly / object file");
  cout << endl;
  cout << "\tAvailable features:" << endl;
#define DEF_TARGET_FEATURE(target, id, key, description)        \
  cout << "\t" << setw(6) << right << key                               \
       << left << ": (" << TargetArchTable[MK_TARGET_ARCH(target)].Name << ") " \
       << description << endl;
#include "target.inc"

  cout << endl;

  return;
}

#undef OUTPUT_OPTION
