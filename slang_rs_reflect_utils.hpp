#ifndef _SLANG_COMPILER_SLANG_REFLECT_UTILS_HPP
#define _SLANG_COMPILER_SLANG_REFLECT_UTILS_HPP

#include <string>

namespace slang {

class RSSlangReflectUtils {
public:
   // Compuate a Java source file path from a given prefixPath and its package.
   // Eg, given prefixPath=./foo/bar and packageName=com.x.y, then it returns
   // ./foo/bar/com/x/y
   static std::string ComputePackagedPath(const std::string& prefixPath,
                                          const std::string& packageName);

   // Compute Java class name from a .rs file name.
   // Any non-alnum character will be discarded. The result will be camel-cased.
   // Eg, with rsFileName=./foo/bar/my_renderscript_file.rs it returns
   // "MyRenderscriptFile".
   // rsFileName: the input .rs file name (with or without path).
   static std::string JavaClassNameFromRSFileName(const char* rsFileName);

   // Compute a bitcode file name (no extension) from a .rs file name.
   // Because the bitcode file name may be used as Resource ID in the generated
   // class (something like R.raw.<bitcode_filename>), Any non-alnum character
   // will be discarded.
   // The difference from JavaClassNameFromRSFileName() is that the result is
   // not converted to camel case.
   // Eg, with rsFileName=./foo/bar/my_renderscript_file.rs it returns
   // "myrenderscriptfile"
   // rsFileName: the input .rs file name (with or without path).
   static std::string BCFileNameFromRSFileName(const char* rsFileName);

   // "mkdir -p"
   static bool mkdir_p(const char* path);

   // Encode a binary bitcode file into a Java source file.
   // rsFileName: the original .rs file name (with or without path).
   // inputBCFileName: the bitcode file to be encoded.
   // outputPath: where to output the generated Java file, no package name in
   // it.
   // packageName: the package of the output Java file.
   static bool EncodeBitcodeToJavaFile(const char* rsFileName,
                                       const char* inputBCFileName,
                                       const std::string& outputPath,
                                       const std::string& packageName);

};

}

#endif  // _SLANG_COMPILER_SLANG_REFLECT_UTILS_HPP
