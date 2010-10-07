#ifndef _SLANG_COMPILER_UTILS_H
#define _SLANG_COMPILER_UTILS_H

#include <string>

namespace llvm {
  class StringRef;
}

namespace slang {

class SlangUtils {
 private:
  SlangUtils() {}

 public:
  static bool CreateDirectoryWithParents(llvm::StringRef Dir,
                                         std::string* Error);
};
}

#endif  // _SLANG_COMPILER_UTILS_H
