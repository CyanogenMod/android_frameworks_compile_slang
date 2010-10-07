#include "slang_utils.h"

#include "llvm/System/Path.h"

using namespace slang;

bool SlangUtils::CreateDirectoryWithParents(llvm::StringRef Dir,
                                            std::string* Error) {
  return !llvm::sys::Path(Dir).createDirectoryOnDisk(/* create_parents = */true,
                                                     Error);
}
