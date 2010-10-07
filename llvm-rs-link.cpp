#include <memory>
#include <string>
#include <vector>

#include "llvm/Linker.h"
#include "llvm/LLVMContext.h"
#include "llvm/Metadata.h"
#include "llvm/Module.h"

#include "llvm/Analysis/Verifier.h"

#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/StandardPasses.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/System/Signals.h"
#include "llvm/System/Path.h"

#include "llvm/Target/TargetData.h"

#include "slang_rs_metadata.h"

using namespace llvm;

static cl::list<std::string>
InputFilenames(cl::Positional, cl::OneOrMore,
               cl::desc("<input bitcode files>"));

static cl::opt<std::string>
OutputFilename("o", cl::Required, cl::desc("Override output filename"),
               cl::value_desc("<output bitcode file>"));

static bool GetExportSymbolNames(NamedMDNode *N,
                                 unsigned NameOpIdx,
                                 std::vector<const char *> &Names) {
  if (N == NULL)
    return true;

  for (unsigned i = 0, e = N->getNumOperands(); i != e; ++i) {
    MDNode *V = N->getOperand(i);
    if (V == NULL)
      continue;

    if (V->getNumOperands() < (NameOpIdx + 1)) {
      errs() << "Invalid metadata spec of " << N->getName()
             << " in RenderScript executable. (#op)\n";
      return false;
    }

    MDString *Name = dyn_cast<MDString>(V->getOperand(NameOpIdx));
    if (Name == NULL) {
      errs() << "Invalid metadata spec of " << N->getName()
             << " in RenderScript executable. (#name)\n";
      return false;
    }

    Names.push_back(Name->getString().data());
  }
  return true;
}

static bool GetExportSymbols(Module *M, std::vector<const char *> &Names) {
  bool Result = true;
  // Variables marked as export must be externally visible
  if (NamedMDNode *EV = M->getNamedMetadata(RS_EXPORT_VAR_MN))
    Result |= GetExportSymbolNames(EV, RS_EXPORT_VAR_NAME, Names);
  // So are those exported functions
  if (NamedMDNode *EF = M->getNamedMetadata(RS_EXPORT_FUNC_MN))
    Result |= GetExportSymbolNames(EF, RS_EXPORT_FUNC_NAME, Names);
  return Result;
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
      errs() << argv[0] << ": error loading file '"
             << InputFilenames[i] << "'\n";
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
  Out(new raw_fd_ostream(OutputFilename.c_str(),
                         ErrorInfo,
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
  if (!ModuleDataLayout.empty())
    if (TargetData *TD = new TargetData(ModuleDataLayout))
      Passes.add(TD);

  // Some symbols must not be internalized
  std::vector<const char *> ExportList;
  ExportList.push_back("init");
  ExportList.push_back("root");

  if (!GetExportSymbols(Composite.get(), ExportList))
    return 1;

  Passes.add(createInternalizePass(ExportList));

  createStandardLTOPasses(&Passes,
                          /* Internalize = */false,
                          /* RunInliner = */true,
                          /* VerifyEach = */false);
  Passes.run(*Composite.get());

  WriteBitcodeToFile(Composite.get(), *Out);

  return 0;
}
