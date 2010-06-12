#include "libslang.h"

#include <assert.h>
#include <getopt.h>

#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <iostream>

using namespace std;

#define ERR_NO_INPUT_FILE           "no input file"

#define WARN(x) cerr << "warning: " WARN_ ## x << endl
#define WARN1(x, v1) cerr << "warning: " WARN_ ## x(v1) << endl
#define WARN2(x, v1, v2) cerr << "warning: " WARN_ ## x(v1, v2) << endl

#define WARN_MULTIPLE_INPUT_FILES   "multiple input files is not supported currently, only first input file will be compiled"

#define WARN_UNKNOWN_CPU(v1)            "the given CPU " << (v1) << " cannot be recognized, but we'll force passing it to Slang compiler"

#define WARN_MISMATCH_CPU_TARGET_ARCH(v1, v2)   \
    "CPU (target: " << (v1) << ") you selected doesn't match the target of enable features you specified or the triple string you given (" << (v2) << ")"
#define WARN_MISMATCH_FEATURE_TARGET_ARCH(v1, v2)   \
    "Feature (target: " << (v1) << ") you selected doesn't match the target of CPU you specified or the triple string you given (" << (v2) << ")"

#define DEFAULT_OUTPUT_FILENAME     "a.out"

/* List of all support target, will look like "ArchARM" */
#define MK_TARGET_ARCH(target)  Arch ## target
typedef enum {
    ArchNone,
#define DEF_SUPPORT_TARGET(target, name, default_triple)  \
    MK_TARGET_ARCH(target),
#   include "target.inc"

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
#   include "target.inc"
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
    target ## FeatureStart,    \
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
#   include "target.inc"

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
#   include "target.inc"
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
static SlangCompilerOutputTy OutputFileType;
static const char* OutputFileName;
static const char* JavaReflectionPackageName;
static const char* InputFileName;
static bool Verbose;
static const char* FeatureEnabledList[MaxTargetFeature + 1];

/* Construct the command options table used in ParseOption::getopt_long */
static void ConstructCommandOptions() {
    /* Basic slang command option */
    static struct option BasicSlangOpts[] = {
        { "emit-llvm",      no_argument, (int*) &OutputFileType, SlangCompilerOutput_LL },
        { "emit-bc",        no_argument, (int*) &OutputFileType, SlangCompilerOutput_Bitcode },
        { "emit-asm",       no_argument, NULL, 'S' },
        { "emit-obj",       no_argument, NULL, 'c' },
        { "emit-nothing",   no_argument, (int*) &OutputFileType, SlangCompilerOutput_Nothing },

        { "help",    no_argument, NULL, 'h' }, /* -h */
        { "verbose", no_argument, NULL, 'v' }, /* -v */

        { "output", required_argument, NULL, 'o' }, /* -o */
        { "cpu",    required_argument, NULL, 'u' }, /* -u */
        { "triple", required_argument, NULL, 't' }, /* -t */

        { "output-java-reflection-class", required_argument, NULL, 'j'}   /* -j */
    };

    const int NumberOfBasicOptions = sizeof(BasicSlangOpts) / sizeof(struct option);

    SlangOpts = new struct option [ NumberOfBasicOptions + MaxTargetFeature * 2 /* for --enable-feature and --disable-feature */ ];

    /* Fill SlangOpts with basic options */
    memcpy(SlangOpts, BasicSlangOpts, sizeof(BasicSlangOpts));

    int i = NumberOfBasicOptions;
    /* Add --enable-TARGET_FEATURE option into slang command option */
#define DEF_TARGET_FEATURE(target, id, key, description)  \
        SlangOpts[i].name = "enable-" key;   \
        SlangOpts[i].has_arg = optional_argument;   \
        SlangOpts[i].flag = (int*) &EnableFeatureValue;  \
        SlangOpts[i].val = target ## id;    \
        i++;
#   include "target.inc"

    /* Add --disable-TARGET_FEATURE option into slang command option */
#define DEF_TARGET_FEATURE(target, id, key, description)  \
        SlangOpts[i].name = "disable-" key;   \
        SlangOpts[i].has_arg = optional_argument;   \
        SlangOpts[i].flag = (int*) &DisableFeatureValue;    \
        SlangOpts[i].val = target ## id;    \
        i++;
#   include "target.inc"

    /* NULL-terminated the SlangOpts */
    memset(&SlangOpts[i], 0, sizeof(struct option));

    return;
}

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;

static void Usage(const char* CommandName) {
#define OUTPUT_OPTION(short_name, long_name, desc) \
    do {    \
        if(short_name)  \
            cout << setw(4) << right << (short_name) << ", ";    \
        else    \
            cout << "      ";   \
        cout << setw(17) << left << (long_name);    \
        cout << " " << (desc) << endl; \
    } while(false)

    cout << "Usage: " << CommandName << " [OPTION]... " << "[INPUT FILE]" << endl;

    cout << endl;

    cout << "Basic: " << endl;

    OUTPUT_OPTION("-h", "--help", "print this help");
    OUTPUT_OPTION("-v", "--verbos", "be verbose");
    OUTPUT_OPTION("-o", "--output=<FILE>", "write the output of compilation to FILE ('-' means stdout)");
    OUTPUT_OPTION("-j", "--output-java-reflection-package=<PACKAGE NAME>", "output reflection to Java for BCC exportables");

    cout << endl;

    cout << "Output type:" << endl;

    OUTPUT_OPTION(NULL, "--emit-llvm", "set output type to LLVM assembly (.ll)");
    OUTPUT_OPTION(NULL, "--emit-bc", "set output type to Bitcode (.bc) (Default)");
    OUTPUT_OPTION("-S", "--emit-asm", "set output type to target assmbly code (.S)");
    OUTPUT_OPTION("-c", "--emit-obj", "set output type to target object file (.o)");
    OUTPUT_OPTION(NULL, "--emit-nothing", "output nothing");

    cout << endl;

    cout << "Code generation option: " << endl;

    OUTPUT_OPTION("-u", "--cpu=CPU", "generate the assembly / object file for the CPU");
    cout << endl;
    cout << "\tAvailable CPU:" << endl;

    for(int i=0;i<NUM_TARGET_CPU;i++) 
        cout << "\t" << setw(13) << right << TargetCPUTable[i].Name << left << ": (" << TargetArchTable[(TargetCPUTable[i].Arch)].Name << ") " << TargetCPUTable[i].Desc << endl;

    cout << endl;

    OUTPUT_OPTION("-t", "--triple=TRIPLE", "generate the assembly / object file for the Triple");
    cout << "\tDefault triple: " << endl;
#define DEF_SUPPORT_TARGET(target, name, default_triple)    \
        cout << "\t" << setw(5) << right << name << left << ": " << default_triple << endl;
#include "target.inc"
    cout << endl;

    OUTPUT_OPTION(NULL, "--enable-FEATURE", "enable the FEATURE for the generation of the assembly / object file");
    OUTPUT_OPTION(NULL, "--disable-FEATURE", "disable the FEATURE for the generation of the assembly / object file");
    cout << endl;
    cout << "\tAvailable features:" << endl;
#define DEF_TARGET_FEATURE(target, id, key, description)    \
    cout << "\t" << setw(6) << right << key  \
        << left << ": (" << TargetArchTable[MK_TARGET_ARCH(target)].Name << ") " \
        << description << endl;
#include "target.inc"


    cout << endl;


#undef OUTPUT_OPTION
    return;
}

static bool ParseOption(int Argc, char** Argv) {
    assert(SlangOpts != NULL && "Slang command options table was not initialized!");

    /* Set default value to option */
    CPU = NULL;
    TripleString = DEFAULT_TARGET_TRIPLE_STRING;
    EnableFeatureValue = DisableFeatureValue = FeatureNone;
    OutputFileType = SlangCompilerOutput_Default;
    OutputFileName = DEFAULT_OUTPUT_FILENAME;
    JavaReflectionPackageName = NULL;
    InputFileName = NULL;
    Verbose = false;
    FeatureEnabledList[0] = NULL;

    int ch;

    unsigned int FeatureEnableBits = 0;
    unsigned int FeatureDisableBits = 0;
#define ENABLE_FEATURE(x)   \
    FeatureEnableBits |= (x)
#define DISABLE_FEATURE(x)   \
    FeatureDisableBits |= (x)
    TargetArchEnum ExpectedArch = ArchNone;

    /* Turn off the error message output by getopt_long */
    opterr = 0;

    while((ch = getopt_long(Argc, Argv, "Schvo:u:t:j:", SlangOpts, NULL)) != -1) {
        switch(ch) {
            case 'S':
                OutputFileType = SlangCompilerOutput_Assembly;
            break;

            case 'c':
                OutputFileType = SlangCompilerOutput_Obj;
            break;

            case 'o':
                OutputFileName = optarg;
            break;

            case 'j':
                JavaReflectionPackageName = optarg;
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

#define DEF_TARGET_FEATURE(target, id, key, description)    \
        if(FeatureDisableBits & MK_TARGET_FEATURE_BIT(target, id))  \
            FeatureEnabledList[ CurFeatureEnableListIdx++ ] = "-" key;  \
        else if(FeatureEnableBits & MK_TARGET_FEATURE_BIT(target, id))   \
            FeatureEnabledList[ CurFeatureEnableListIdx++ ] = "+" key;
#define HOOK_TARGET_FIRST_FEATURE(target, id, key, description) \
        case Arch ## target:    \
            /* Fix target triple */ \
            if(TripleString == DEFAULT_TARGET_TRIPLE_STRING)  \
                TripleString = TargetArchTable[MK_TARGET_ARCH(target)].DefaultTriple; \
            DEF_TARGET_FEATURE(target, id, key, description)
#define HOOK_TARGET_LAST_FEATURE(target, id, key, description) \
            DEF_TARGET_FEATURE(target, id, key, description)    \
            FeatureEnabledList[ CurFeatureEnableListIdx++ ] = NULL; /* null-terminator */   \
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

    if(Argc > 1) 
        WARN(MULTIPLE_INPUT_FILES);
    InputFileName = Argv[optind];

    if(Verbose) {
        cout << "Input: " << InputFileName << endl;

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

        cout << "Output to: " << ((strcmp(OutputFileName, "-")) ? OutputFileName : "(standard output)") << ", type: ";
        switch(OutputFileType) {
            case SlangCompilerOutput_Assembly: cout << "Target Assembly"; break;
            case SlangCompilerOutput_LL: cout << "LLVM Assembly"; break;
            case SlangCompilerOutput_Bitcode: cout << "Bitcode"; break;
            case SlangCompilerOutput_Nothing: cout << "No output (test)"; break;
            case SlangCompilerOutput_Obj: cout << "Object file"; break;
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

#define SLANG_CALL_AND_CHECK(expr)   \
    if(!(expr)) { \
        if(slangGetInfoLog(slang))  \
            cerr << slangGetInfoLog(slang); \
        goto on_slang_error;    \
    } 

int main(int argc, char** argv) {
    if(argc < 2) {
        cerr << argv[0] << ": "ERR_NO_INPUT_FILE << endl;
        return 1;
    }

    ConstructCommandOptions();

    if(ParseOption(argc, argv)) {
        /* Start compilation */
        SlangCompiler* slang = slangCreateCompiler(TripleString, CPUString, FeatureEnabledList);
        if(slang != NULL) {
            SLANG_CALL_AND_CHECK( slangSetSourceFromFile(slang, InputFileName) );

            slangSetOutputType(slang, OutputFileType);

            SLANG_CALL_AND_CHECK( slangSetOutputToFile(slang, OutputFileName) );

            SLANG_CALL_AND_CHECK( slangCompile(slang) <= 0 );

            /* output log anyway */
            if(slangGetInfoLog(slang))
                cout << slangGetInfoLog(slang);

            SLANG_CALL_AND_CHECK( slangReflectToJava(slang, JavaReflectionPackageName) );

on_slang_error:
            delete slang;
        }
    }

    DestroyCommandOptions();

    return 0;
}
