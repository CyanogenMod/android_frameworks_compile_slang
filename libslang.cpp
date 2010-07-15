#include "slang.hpp"

namespace slang {

/* Following are the API we provide for using slang compiler */
extern "C" SlangCompiler* slangCreateCompiler(const char* targetTriple, const char* targetCPU, const char** targetFeatures) {
    Slang* slang = new Slang(targetTriple, targetCPU, targetFeatures);
    return (SlangCompiler*) slang;
}

extern "C" void slangAllowRSPrefix(SlangCompiler* compiler) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        slang->allowRSPrefix();
}

extern "C" int slangSetSourceFromMemory(SlangCompiler* compiler, const char* text, size_t textLength) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->setInputSource("<in memory>", text, textLength);
    else
        return 0;
}

extern "C" int slangSetSourceFromFile(SlangCompiler* compiler, const char* fileName) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->setInputSource(fileName);
    else
        return 0;
}

extern "C" void slangSetOutputType(SlangCompiler* compiler, SlangCompilerOutputTy outputType) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        slang->setOutputType(outputType);
    return;
}

extern "C" int slangSetOutputToStream(SlangCompiler* compiler, FILE* stream) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->setOutput(stream);
    else
        return 0;
}

extern "C" int slangSetOutputToFile(SlangCompiler* compiler, const char* fileName) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->setOutput(fileName);
    else
        return 0;
}

extern "C" int slangCompile(SlangCompiler* compiler) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->compile();
    else
        return 0;
}

extern "C" int slangReflectToJava(SlangCompiler* compiler, const char* packageName) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->reflectToJava(packageName);
    else
        return 0;
}

extern "C" int slangReflectToJavaPath(SlangCompiler* compiler, const char* pathName) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->reflectToJavaPath(pathName);
    else
        return 0;
}

extern "C" const char* slangGetInfoLog(SlangCompiler* compiler) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        return slang->getErrorMessage();
    else
        return "";
}

extern "C" void slangGetPragmas(SlangCompiler* compiler, size_t* actualStringCount, size_t maxStringCount, char** strings) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        slang->getPragmas(actualStringCount, maxStringCount, strings);
    return;
}

extern "C" void slangReset(SlangCompiler* compiler) {
    Slang* slang = (Slang*) compiler;
    if(slang != NULL)
        slang->reset();
    return;
}

}   /* namespace slang */
