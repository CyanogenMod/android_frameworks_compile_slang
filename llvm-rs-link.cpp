#include "llvm/Linker.h"
#include "llvm/LLVMContext.h"
#include "llvm/Metadata.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/StandardPasses.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/System/Signals.h"
#include "llvm/System/Path.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/LinkAllVMCore.h"
#include <memory>
#include <string>
#include <vector>

using namespace llvm;

static cl::list<std::string>
InputFilenames(cl::Positional, cl::OneOrMore,
               cl::desc("<input bitcode files>"));

static cl::opt<std::string>
OutputFilename("o", cl::Required, cl::desc("Override output filename"),
               cl::value_desc("<output bitcode file>"));

static cl::opt<bool>
Externalize("e", cl::Optional, cl::desc("To externalize"));

// GetExported - ...
static void GetExported(NamedMDNode *N, std::vector<std::string> &Names) {
  for (int i = 0, e = N->getNumOperands(); i != e; ++i) {
    MDNode *exported_var = N->getOperand(i);
    if (!exported_var) continue;

    if (exported_var->getNumOperands() == 0) {
      errs() << "Invalid RenderScript reflection data.\n";
      abort();
    }

    MDString *name = dyn_cast<MDString>(exported_var->getOperand(0));
    if (!name) {
      errs() << "Invalid RenderScript reflection data.\n";
      abort();
    }

    Names.push_back(name->getString());
  }
}

// GetExportedGlobals - ...
static void GetExportedGlobals(Module *M, std::vector<std::string> &Names) {
  if (NamedMDNode *rs_export_var = M->getNamedMetadata("#rs_export_var")) {
    GetExported(rs_export_var, Names);
  }
  if (NamedMDNode *rs_export_func = M->getNamedMetadata("#rs_export_func")) {
    GetExported(rs_export_func, Names);
  }
}

// LoadFile - Read the specified bitcode file in and return it.  This routine
// searches the link path for the specified file to try to find it...
//
static inline std::auto_ptr<Module> LoadFile(const char *argv0,
                                             const std::string &FN,
                                             LLVMContext& Context) {
  sys::Path Filename;
  if (!Filename.set(FN)) {
    errs() << "Invalid file name: '" << FN << "'\n";
    return std::auto_ptr<Module>();
  }

  std::string Err;
  Module *Result = 0;

  const std::string &FNStr = Filename.str();
  MemoryBuffer *Buffer = MemoryBuffer::getFile(FNStr, &Err);
  if (!Buffer) {
    errs() << Err;
    return std::auto_ptr<Module>();
  }
  Result = ParseBitcodeFile(Buffer, Context, &Err);
  if (Result) return std::auto_ptr<Module>(Result);   // Load successful!

  errs() << Err;
  return std::auto_ptr<Module>();
}

int main(int argc, char **argv) {
  llvm_shutdown_obj X;

  cl::ParseCommandLineOptions(argc, argv, "llvm-rs-link\n");

  LLVMContext &Context = getGlobalContext();
  std::auto_ptr<Module> Composite = LoadFile(argv[0], InputFilenames[0],
                                             Context);
  std::string ErrorMessage;
  if (Composite.get() == 0) {
    errs() << argv[0] << ": error loading file '"
           << InputFilenames[0] << "'\n";
    return 1;
  }
  for (unsigned i = 1; i < InputFilenames.size(); ++i) {
    std::auto_ptr<Module> M(LoadFile(argv[0], InputFilenames[i], Context));
    if (M.get() == 0) {
      errs() << argv[0] << ": error loading file '" <<InputFilenames[i]<< "'\n";
      return 1;
    }
    if (Linker::LinkModules(Composite.get(), M.get(), &ErrorMessage)) {
      errs() << argv[0] << ": link error in '" << InputFilenames[i]
             << "': " << ErrorMessage << "\n";
      return 1;
    }
  }

  std::string ErrorInfo;
  std::auto_ptr<raw_ostream>
  Out(new raw_fd_ostream(OutputFilename.c_str(), ErrorInfo,
                         raw_fd_ostream::F_Binary));
  if (!ErrorInfo.empty()) {
    errs() << ErrorInfo << '\n';
    return 1;
  }

  if (verifyModule(*Composite)) {
    errs() << argv[0] << ": linked module is broken!\n";
    return 1;
  }

  PassManager Passes;

  const std::string &ModuleDataLayout = Composite.get()->getDataLayout();
  if (!ModuleDataLayout.empty()) {
    if (TargetData *TD = new TargetData(ModuleDataLayout))
      Passes.add(TD);
  }

  if (!Externalize) {
    std::vector<std::string> PublicAPINames;
    PublicAPINames.push_back("init");
    PublicAPINames.push_back("root");
    GetExportedGlobals(Composite.get(), PublicAPINames);

    std::vector<const char *> PublicAPINamesCStr;
    for (int i = 0, e = PublicAPINames.size(); i != e; ++i) {
      // errs() << "not internalizing: " << PublicAPINames[i] << '\n';
      PublicAPINamesCStr.push_back(PublicAPINames[i].c_str());
    }

    Passes.add(createInternalizePass(PublicAPINamesCStr));
  }

  createStandardLTOPasses(&Passes, false, true, false);
  Passes.run(*Composite.get());

  WriteBitcodeToFile(Composite.get(), *Out);
  return 0;
}
