#ifndef _ANDROID_SLANG_H_
#   define _ANDROID_SLANG_H_

#include <stdio.h>

// Hide the underlying C++ implementation of Slang
typedef int SlangCompiler;

typedef enum {
  SlangCompilerOutput_Assembly,
  SlangCompilerOutput_LL,
  SlangCompilerOutput_Bitcode,
  SlangCompilerOutput_Nothing,
  SlangCompilerOutput_Obj,

  SlangCompilerOutput_Default = SlangCompilerOutput_Bitcode
} SlangCompilerOutputTy;

#ifdef __cplusplus
extern "C" {
#endif

SlangCompiler* slangCreateCompiler(const char *targetTriple,
                                   const char *targetCPU,
                                   const char **targetFeatures);

void slangAllowRSPrefix(SlangCompiler *compiler);

int slangSetSourceFromMemory(SlangCompiler *compiler,
                             const char *text,
                             size_t textLength);
int slangSetSourceFromFile(SlangCompiler *compiler,
                           const char* fileName);

void slangSetOutputType(SlangCompiler *compiler,
                        SlangCompilerOutputTy outputType);

int slangSetOutputToStream(SlangCompiler *compiler, FILE *stream);
int slangSetOutputToFile(SlangCompiler *compiler, const char *fileName);

void slangAddIncludePath(SlangCompiler *compiler, const char *path);

int slangCompile(SlangCompiler *compiler);

// realPackageName: the buffer to receive the package name that's really
// applied, since the package name may be specified in the .rs file.
// bSize: size of the buffer outputPackageName.
int slangReflectToJava(SlangCompiler *compiler,
                       const char *packageName,
                       char realPackageName[],
                       int bSize);
int slangReflectToJavaPath(SlangCompiler *compiler,
                           const char *pathName);

const char* slangGetInfoLog(SlangCompiler *compiler);
void slangGetPragmas(SlangCompiler *compiler,
                     size_t *actualStringCount,
                     size_t maxStringCount,
                     char **strings);

const char* slangExportFuncs(SlangCompiler *compiler);

void slangReset(SlangCompiler *compiler);

#ifdef __cplusplus
}
#endif


#endif  // _ANDROID_SLANG_H_
