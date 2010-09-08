#include "slang_rs_reflect_utils.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>

#include <sys/stat.h>
#include <sys/types.h>

namespace slang {

using std::string;

string RSSlangReflectUtils::ComputePackagedPath(
    const std::string& prefixPath, const std::string& packageName) {
    string packaged_path(prefixPath);
    if (!prefixPath.empty() && (prefixPath[prefixPath.length() - 1] != '/')) {
        packaged_path += "/";
    }
    size_t s = packaged_path.length();
    packaged_path += packageName;
    while (s < packaged_path.length()) {
        if (packaged_path[s] == '.') {
            packaged_path[s] = '/';
        }
        ++s;
    }
    return packaged_path;
}

static string InternalFileNameConvert(const char* rsFileName, bool camelCase) {
    const char* dot = rsFileName + strlen(rsFileName);
    const char* slash = dot - 1;
    while (slash >= rsFileName) {
        if (*slash == '/') {
            break;
        }
        if ((*slash == '.') && (*dot == 0)) {
            dot = slash;
        }
        --slash;
    }
    ++slash;
    char ret[256];
    bool need_cap = true;
    int i = 0;
    for (; (i < 255) && (slash < dot); ++slash) {
        if (isalnum(*slash)) {
            if (need_cap && camelCase) {
                ret[i] = toupper(*slash);
            } else {
                ret[i] = *slash;
            }
            need_cap = false;
            ++i;
        } else {
            need_cap = true;
        }
    }
    ret[i] = 0;
    return string(ret);
}

std::string RSSlangReflectUtils::JavaClassNameFromRSFileName(
    const char* rsFileName) {
    return InternalFileNameConvert(rsFileName, true);
}


std::string RSSlangReflectUtils::BCFileNameFromRSFileName(
    const char* rsFileName) {
    return InternalFileNameConvert(rsFileName, false);
}

bool RSSlangReflectUtils::mkdir_p(const char* path) {
    char buf[256];
    char *tmp, *p = NULL;
    size_t len = strlen(path);

    if (len + 1 <= sizeof(buf))
        tmp = buf;
    else
        tmp = new char [len + 1];

    strcpy(tmp, path);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(tmp, S_IRWXU);

    if (tmp != buf)
        delete[] tmp;

    return true;
}

bool RSSlangReflectUtils::EncodeBitcodeToJavaFile(
    const char* rsFileName, const char* inputBCFileName,
    const std::string& outputPath, const std::string& packageName) {

    FILE* pfin = fopen(inputBCFileName, "rb");
    if (pfin == NULL) {
        return false;
    }

    string output_path = ComputePackagedPath(outputPath, packageName);
    if (!mkdir_p(output_path.c_str())) {
      return false;
    }

    string clazz_name(JavaClassNameFromRSFileName(rsFileName));
    clazz_name += "BitCode";
    string filename(clazz_name);
    filename += ".java";

    string output_filename(output_path);
    output_filename += "/";
    output_filename += filename;
    printf("Generating %s ...\n", filename.c_str());
    FILE* pfout = fopen(output_filename.c_str(), "w");
    if (pfout == NULL) {
        return false;
    }

    // Output the header
    fprintf(pfout, "/*\n");
    fprintf(pfout, " * This file is auto-generated. DO NOT MODIFY!\n");
    fprintf(pfout, " * The source RenderScript file: %s\n", rsFileName);
    fprintf(pfout, " */\n\n");
    fprintf(pfout, "package %s;\n\n", packageName.c_str());
    fprintf(pfout, "/**\n");
    fprintf(pfout, " * @hide\n");
    fprintf(pfout, " */\n");
    fprintf(pfout, "public class %s {\n", clazz_name.c_str());
    fprintf(pfout, "\n");
    fprintf(pfout, "  // return byte array representation of the bitcode file.\n");
    fprintf(pfout, "  public static byte[] getBitCode() {\n");
    fprintf(pfout, "    byte[] bc = new byte[data.length];\n");
    fprintf(pfout, "    System.arraycopy(data, 0, bc, 0, data.length);\n");
    fprintf(pfout, "    return bc;\n");
    fprintf(pfout, "  }\n");
    fprintf(pfout, "\n");
    fprintf(pfout, "  // byte array representation of the bitcode file.\n");
    fprintf(pfout, "  private static final byte[] data = {\n");

    // output the data
    const static int BUFF_SIZE = 0x10000;
    char* buff = new char[BUFF_SIZE];
    int read_length;
    while ((read_length = fread(buff, 1, BUFF_SIZE, pfin)) > 0) {
        const static int LINE_BYTE_NUM = 16;
        char out_line[LINE_BYTE_NUM*6 + 10];
        const char* out_line_end = out_line + sizeof(out_line);
        char* p = out_line;

        int write_length = 0;
        while (write_length < read_length) {
            p += snprintf(p, out_line_end - p, " %4d,", (int)buff[write_length]);
            ++write_length;
            if (((write_length % LINE_BYTE_NUM) == 0)
                || (write_length == read_length)) {
                fprintf(pfout, "   ");
                fprintf(pfout, out_line);
                fprintf(pfout, "\n");
                p = out_line;
            }
        }

    }
    delete []buff;

    // the rest of the java file.
    fprintf(pfout, "  };\n");
    fprintf(pfout, "}\n");

    fclose(pfin);
    fclose(pfout);
    return true;
}

}
