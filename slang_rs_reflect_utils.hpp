#ifndef _SLANG_COMPILER_SLANG_REFLECT_UTILS_HPP
#define _SLANG_COMPILER_SLANG_REFLECT_UTILS_HPP

#include <string>

namespace slang {

// BitCode storage type
enum BitCodeStorageType {
  BCST_APK_RESOURCE,
  BCST_JAVA_CODE
};

class RSSlangReflectUtils {
 public:

  // Encode a binary bitcode file into a Java source file.
  // rsFileName: the original .rs file name (with or without path).
  // bcFileName: where is the bit code file
  // reflectPath: where to output the generated Java file, no package name in
  // it.
  // packageName: the package of the output Java file.
  struct BitCodeAccessorContext {
    const char* rsFileName;
    const char* bcFileName;
    const char* reflectPath;
    const char* packageName;

    BitCodeStorageType bcStorage;
  };

  // Compuate a Java source file path from a given prefixPath and its package.
  // Eg, given prefixPath=./foo/bar and packageName=com.x.y, then it returns
  // ./foo/bar/com/x/y
  static std::string ComputePackagedPath(const char* prefixPath,
                                         const char* packageName);

  // Compute Java class name from a .rs file name.
  // Any non-alnum, non-underscore characters will be discarded.
  // E.g. with rsFileName=./foo/bar/my-Renderscript_file.rs it returns
  // "myRenderscript_file".
  // rsFileName: the input .rs file name (with or without path).
  static std::string JavaClassNameFromRSFileName(const char* rsFileName);

  // Compute a bitcode file name (no extension) from a .rs file name.
  // Because the bitcode file name may be used as Resource ID in the generated
  // class (something like R.raw.<bitcode_filename>), Any non-alnum,
  // non-underscore character will be discarded.
  // The difference from JavaClassNameFromRSFileName() is that the result is
  // converted to lowercase.
  // E.g. with rsFileName=./foo/bar/my-Renderscript_file.rs it returns
  // "myrenderscript_file"
  // rsFileName: the input .rs file name (with or without path).
  static std::string BCFileNameFromRSFileName(const char* rsFileName);

  // "mkdir -p"
  static bool mkdir_p(const char* path);


  // Generate the bit code accessor Java source file.
  static bool GenerateBitCodeAccessor(const BitCodeAccessorContext& context);

};

}

#endif  // _SLANG_COMPILER_SLANG_REFLECT_UTILS_HPP
