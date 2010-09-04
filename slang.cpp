#include "slang.hpp"
#include "slang_rs_export_func.hpp"

#include <stdlib.h>                     /* for getenv */

#include "libslang.h"

#include "llvm/ADT/Twine.h"     /* for class llvm::Twine */

#include "llvm/Target/TargetSelect.h"       /* for function LLVMInitialize[ARM|X86][TargetInfo|Target|AsmPrinter]() */

#include "llvm/Support/MemoryBuffer.h"      /* for class llvm::MemoryBuffer */
#include "llvm/Support/ErrorHandling.h"     /* for function llvm::install_fatal_error_handler() */
#include "llvm/Support/ManagedStatic.h"     /* for class llvm::llvm_shutdown */

#include "clang/Basic/TargetInfo.h"     /* for class clang::TargetInfo */
#include "clang/Basic/LangOptions.h"    /* for class clang::LangOptions */
#include "clang/Basic/TargetOptions.h"  /* for class clang::TargetOptions */

#include "clang/Frontend/FrontendDiagnostic.h"      /* for clang::diag::* */

#include "clang/Sema/ParseAST.h"        /* for function clang::ParseAST() */

#if defined(__arm__)
#   define DEFAULT_TARGET_TRIPLE_STRING "armv7-none-linux-gnueabi"
#elif defined(__x86_64__)
#   define DEFAULT_TARGET_TRIPLE_STRING "x86_64-unknown-linux"
#else
#   define DEFAULT_TARGET_TRIPLE_STRING "i686-unknown-linux"    // let's use x86 as default target
#endif

namespace slang {

bool Slang::GlobalInitialized = false;

/* Language option (define the language feature for compiler such as C99) */
LangOptions Slang::LangOpts;

/* Code generation option for the compiler */
CodeGenOptions Slang::CodeGenOpts;

const std::string Slang::TargetDescription = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-n32";

/* The named of metadata node that pragma resides (should be synced with bcc.cpp) */
const llvm::Twine Slang::PragmaMetadataName = "#pragma";

void Slang::GlobalInitialization() {
    if(!GlobalInitialized) {
        /* We only support x86, x64 and ARM target */

        /* For ARM */
        LLVMInitializeARMTargetInfo();
        LLVMInitializeARMTarget();
        LLVMInitializeARMAsmPrinter();

        /* For x86 and x64 */
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86Target();
        LLVMInitializeX86AsmPrinter();

        /* Please refer to clang/include/clang/Basic/LangOptions.h for setting up the options */
        LangOpts.RTTI = 0;  /* turn off the RTTI information support */
        LangOpts.NeXTRuntime = 0;   /* turn off the NeXT runtime uses */
        LangOpts.Bool = 1;  /* turn on 'bool', 'true', 'false' keywords. */

        CodeGenOpts.OptimizationLevel = 3;  /* -O3 */

        GlobalInitialized = true;
    }

    return;
}

void Slang::LLVMErrorHandler(void *UserData, const std::string &Message) {
    Diagnostic* Diags = static_cast<Diagnostic*>(UserData);
    Diags->Report(clang::diag::err_fe_error_backend) << Message;
    exit(1);
}

void Slang::createTarget(const char* Triple, const char* CPU, const char** Features) {
    if(Triple != NULL)
        mTargetOpts.Triple = Triple;
    else
        mTargetOpts.Triple = DEFAULT_TARGET_TRIPLE_STRING;

    if(CPU != NULL)
        mTargetOpts.CPU = CPU;

    mTarget.reset(TargetInfo::CreateTargetInfo(*mDiagnostics, mTargetOpts));

    if(Features != NULL)
        for(int i=0;Features[i]!=NULL;i++)
            mTargetOpts.Features.push_back(Features[i]);

    return;
}

void Slang::createPreprocessor() {
  HeaderSearch* HS = new HeaderSearch(*mFileMgr); /* Default only search header file in current dir */

  mPP.reset(new Preprocessor( *mDiagnostics,
                              LangOpts,
                              *mTarget,
                              *mSourceMgr,
                              *HS,
                              NULL,
                              true /* OwnsHeaderSearch */));
  /* Initialize the prepocessor */
  mPragmas.clear();
  mPP->AddPragmaHandler(NULL, new PragmaRecorder(mPragmas));

  std::string inclFiles("#include \"rs_types.rsh\"");
  mPP->setPredefines(inclFiles + "\n" + "#include \"rs_math.rsh\"" + "\n");

  std::vector<DirectoryLookup> SearchList;
  for (int i = 0; i < mIncludePaths.size(); ++i) {
      if (const DirectoryEntry *DE = mFileMgr->getDirectory(mIncludePaths[i])) {
          SearchList.push_back(DirectoryLookup(DE, SrcMgr::C_System, false, false));
      }
  }

  HS->SetSearchPaths(SearchList, 1, false);

  return;
}

Slang::Slang(const char* Triple, const char* CPU, const char** Features) :
    mOutputType(SlangCompilerOutput_Default),
    mAllowRSPrefix(false)
{
    GlobalInitialization();

    createDiagnostic();
    llvm::install_fatal_error_handler(LLVMErrorHandler, mDiagnostics.get());

    createTarget(Triple, CPU, Features);
    createFileManager();
    createSourceManager();

    return;
}

bool Slang::setInputSource(llvm::StringRef inputFile, const char* text, size_t textLength) {
    mInputFileName = inputFile.str();

    /* Reset the ID tables if we are reusing the SourceManager */
    mSourceMgr->clearIDTables();

    /* Load the source */
    llvm::MemoryBuffer *SB = llvm::MemoryBuffer::getMemBuffer(text, text + textLength);
    mSourceMgr->createMainFileIDForMemBuffer(SB);

    if(mSourceMgr->getMainFileID().isInvalid()) {
        mDiagnostics->Report(clang::diag::err_fe_error_reading) << inputFile;
        return false;
    }

    return true;
}

bool Slang::setInputSource(llvm::StringRef inputFile) {
    mInputFileName = inputFile.str();

    mSourceMgr->clearIDTables();

    const FileEntry* File = mFileMgr->getFile(inputFile);
    if(File)
        mSourceMgr->createMainFileID(File, SourceLocation());

    if(mSourceMgr->getMainFileID().isInvalid()) {
        mDiagnostics->Report(clang::diag::err_fe_error_reading) << inputFile;
        return false;
    }

    return true;
}

void Slang::addIncludePath(const char* path) {
    mIncludePaths.push_back(path);
}

void Slang::setOutputType(SlangCompilerOutputTy outputType) {
    mOutputType = outputType;
    if( mOutputType != SlangCompilerOutput_Assembly &&
        mOutputType != SlangCompilerOutput_LL &&
        mOutputType != SlangCompilerOutput_Bitcode &&
        mOutputType != SlangCompilerOutput_Nothing &&
        mOutputType != SlangCompilerOutput_Obj)
        mOutputType = SlangCompilerOutput_Default;
    return;
}

static void _mkdir_given_a_file(const char *file) {
    char buf[256];
    char *tmp, *p = NULL;
    size_t len = strlen(file);

    if (len + 1 <= sizeof(buf))
        tmp = buf;
    else
        tmp = new char [len + 1];

    strcpy(tmp, file);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }

    if (tmp != buf)
        delete[] tmp;
}

bool Slang::setOutput(const char* outputFile) {
    std::string Error;

    _mkdir_given_a_file(outputFile);

    switch(mOutputType) {
        case SlangCompilerOutput_Assembly:
        case SlangCompilerOutput_LL:
            mOS.reset( new llvm::raw_fd_ostream(outputFile, Error, 0) );
        break;

        case SlangCompilerOutput_Nothing:
            mOS.reset();
        break;

        case SlangCompilerOutput_Obj:
        case SlangCompilerOutput_Bitcode:
        default:
            mOS.reset( new llvm::raw_fd_ostream(outputFile, Error, llvm::raw_fd_ostream::F_Binary) );
        break;
    }

    if(!Error.empty()) {
        mOS.reset();
        mDiagnostics->Report(clang::diag::err_fe_error_opening) << outputFile << Error;
        return false;
    }

    mOutputFileName = outputFile;

    return true;
}

int Slang::compile() {
    if((mDiagnostics->getNumErrors() > 0) || (mOS.get() == NULL))
        return mDiagnostics->getNumErrors();

    /* Here is per-compilation needed initialization */
    createPreprocessor();
    createASTContext();
    createRSContext();
    //createBackend();
    createRSBackend();

    /* Inform the diagnostic client we are processing a source file */
    mDiagClient->BeginSourceFile(LangOpts, mPP.get());

    /* The core of the slang compiler */
    ParseAST(*mPP, mBackend.get(), *mASTContext);

    /* The compilation ended, clear up */
    mBackend.reset();
    // Can't reset yet because the reflection later on still needs mRSContext
    //    mRSContext.reset();
    mASTContext.reset();
    mPP.reset();

    /* Inform the diagnostic client we are done with previous source file */
    mDiagClient->EndSourceFile();

    return mDiagnostics->getNumErrors();
}

bool Slang::reflectToJava(const char* outputPackageName, char* realPackageName, int bSize) {
    if(mRSContext.get())
        return mRSContext->reflectToJava(outputPackageName, mInputFileName, mOutputFileName,
                                         realPackageName, bSize);
    else
        return false;
}

bool Slang::reflectToJavaPath(const char* outputPathName) {
    if(mRSContext.get())
        return mRSContext->reflectToJavaPath(outputPathName);
    else
        return false;
}

void Slang::getPragmas(size_t* actualStringCount, size_t maxStringCount, char** strings) {
    int stringCount = mPragmas.size() * 2;

    if(actualStringCount)
        *actualStringCount = stringCount;
    if(stringCount > maxStringCount)
        stringCount = maxStringCount;
    if(strings)
        for(PragmaList::const_iterator it = mPragmas.begin();
            stringCount > 0;
            stringCount-=2, it++)
        {
            *strings++ = const_cast<char*>(it->first.c_str());
            *strings++ = const_cast<char*>(it->second.c_str());
        }

    return;
}

typedef std::list<RSExportFunc*> ExportFuncList;

const char* Slang::exportFuncs() {
  std::string fNames;
  for (RSContext::const_export_func_iterator I = mRSContext->export_funcs_begin();
       I != mRSContext->export_funcs_end();
       ++I) {
    RSExportFunc* func = *I;
    fNames.push_back(',');
    fNames.append(func->getName());
  }
  return fNames.c_str();
}

Slang::~Slang() {
    llvm::llvm_shutdown();
    return;
}

}   /* namespace slang */
