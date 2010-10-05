#include "slang_rs_reflect_utils.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>

namespace slang {

using std::string;

string RSSlangReflectUtils::ComputePackagedPath(
    const char *prefixPath, const char *packageName) {
    string packaged_path(prefixPath);
    if (!packaged_path.empty() &&
        (packaged_path[packaged_path.length() - 1] != '/')) {
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

static string InternalFileNameConvert(const char *rsFileName, bool toLower) {
    const char *dot = rsFileName + strlen(rsFileName);
    const char *slash = dot - 1;
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
    int i = 0;
    for (; (i < 255) && (slash < dot); ++slash) {
        if (isalnum(*slash) || *slash == '_') {
            if (toLower) {
                ret[i] = tolower(*slash);
            } else {
                ret[i] = *slash;
            }
            ++i;
        }
    }
    ret[i] = 0;
    return string(ret);
}

std::string RSSlangReflectUtils::JavaClassNameFromRSFileName(
    const char *rsFileName) {
    return InternalFileNameConvert(rsFileName, false);
}


std::string RSSlangReflectUtils::BCFileNameFromRSFileName(
    const char *rsFileName) {
    return InternalFileNameConvert(rsFileName, true);
}

bool RSSlangReflectUtils::mkdir_p(const char *path) {
    char buf[256];
    char *tmp, *p = NULL;
    size_t len = strlen(path);

    if (len + 1 <= sizeof(buf))
        tmp = buf;
    else
        tmp = new char[len + 1];

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

static bool GenerateAccessorHeader(
    const RSSlangReflectUtils::BitCodeAccessorContext &context, FILE *pfout) {
    fprintf(pfout, "/*\n");
    fprintf(pfout, " * This file is auto-generated. DO NOT MODIFY!\n");
    fprintf(pfout, " * The source RenderScript file: %s\n", context.rsFileName);
    fprintf(pfout, " */\n\n");
    fprintf(pfout, "package %s;\n\n", context.packageName);

    // add imports here.

    return true;
}

static bool GenerateAccessorMethodSignature(
    const RSSlangReflectUtils::BitCodeAccessorContext &context, FILE *pfout) {
    // the prototype of the accessor method
    fprintf(pfout, "  // return byte array representation of the bitcode.\n");
    fprintf(pfout, "  public static byte[] getBitCode() {\n");
    return true;
}

// Java method size must not exceed 64k,
// so we have to split the bitcode into multiple segments.
static bool GenerateSegmentMethod(
    const char *buff, int blen, int seg_num, FILE *pfout) {

    fprintf(pfout, "  private static byte[] getSegment_%d() {\n", seg_num);
    fprintf(pfout, "    byte[] data = {\n");

    static const int LINE_BYTE_NUM = 16;
    char out_line[LINE_BYTE_NUM*6 + 10];
    const char *out_line_end = out_line + sizeof(out_line);
    char *p = out_line;

    int write_length = 0;
    while (write_length < blen) {
        p += snprintf(p, out_line_end - p,
                      " %4d,", static_cast<int>(buff[write_length]));
        ++write_length;
        if (((write_length % LINE_BYTE_NUM) == 0)
            || (write_length == blen)) {
          fprintf(pfout, "     ");
          fprintf(pfout, out_line);
          fprintf(pfout, "\n");
          p = out_line;
        }
    }

    fprintf(pfout, "    };\n");
    fprintf(pfout, "    return data;\n");
    fprintf(pfout, "  }\n\n");

    return true;
}

static bool GenerateJavaCodeAccessorMethod(
    const RSSlangReflectUtils::BitCodeAccessorContext &context, FILE *pfout) {
    FILE *pfin = fopen(context.bcFileName, "rb");
    if (pfin == NULL) {
        fprintf(stderr, "Error: could not read file %s\n", context.bcFileName);
        return false;
    }

    // start the accessor method
    GenerateAccessorMethodSignature(context, pfout);
    fprintf(pfout, "    return getBitCodeInternal();\n");
    // end the accessor method
    fprintf(pfout, "  };\n\n");

    // output the data
    // make sure the generated function for a segment won't break the Javac
    // size limitation (64K).
    static const int SEG_SIZE = 0x2000;
    char *buff = new char[SEG_SIZE];
    int read_length;
    int seg_num = 0;
    int total_length = 0;
    while ((read_length = fread(buff, 1, SEG_SIZE, pfin)) > 0) {
        GenerateSegmentMethod(buff, read_length, seg_num, pfout);
        ++seg_num;
        total_length += read_length;
    }
    delete []buff;
    fclose(pfin);

    // output the internal accessor method
    fprintf(pfout, "  private static int bitCodeLength = %d;\n\n", total_length);
    fprintf(pfout, "  private static byte[] getBitCodeInternal() {\n");
    fprintf(pfout, "    byte[] bc = new byte[bitCodeLength];\n");
    fprintf(pfout, "    int offset = 0;\n");
    fprintf(pfout, "    byte[] seg;\n");
    for (int i = 0; i < seg_num; ++i) {
    fprintf(pfout, "    seg = getSegment_%d();\n", i);
    fprintf(pfout, "    System.arraycopy(seg, 0, bc, offset, seg.length);\n");
    fprintf(pfout, "    offset += seg.length;\n");
    }
    fprintf(pfout, "    return bc;\n");
    fprintf(pfout, "  }\n\n");

    return true;
}

static bool GenerateAccessorClass(
    const RSSlangReflectUtils::BitCodeAccessorContext &context,
    const char *clazz_name, FILE *pfout) {
    // begin the class.
    fprintf(pfout, "/**\n");
    fprintf(pfout, " * @hide\n");
    fprintf(pfout, " */\n");
    fprintf(pfout, "public class %s {\n", clazz_name);
    fprintf(pfout, "\n");

    bool ret = true;
    switch (context.bcStorage) {
      case BCST_APK_RESOURCE:
        break;
      case BCST_JAVA_CODE:
        ret = GenerateJavaCodeAccessorMethod(context, pfout);
        break;
      default:
        ret = false;
    }

    // end the class.
    fprintf(pfout, "}\n");

    return ret;
}


bool RSSlangReflectUtils::GenerateBitCodeAccessor(
    const BitCodeAccessorContext &context) {
    string output_path = ComputePackagedPath(context.reflectPath,
                                             context.packageName);
    if (!mkdir_p(output_path.c_str())) {
        fprintf(stderr, "Error: could not create dir %s\n",
                output_path.c_str());
        return false;
    }

    string clazz_name(JavaClassNameFromRSFileName(context.rsFileName));
    clazz_name += "BitCode";
    string filename(clazz_name);
    filename += ".java";

    string output_filename(output_path);
    output_filename += "/";
    output_filename += filename;
    printf("Generating %s ...\n", filename.c_str());
    FILE *pfout = fopen(output_filename.c_str(), "w");
    if (pfout == NULL) {
        fprintf(stderr, "Error: could not write to file %s\n",
                output_filename.c_str());
        return false;
    }

    bool ret = GenerateAccessorHeader(context, pfout) &&
               GenerateAccessorClass(context, clazz_name.c_str(), pfout);

    fclose(pfout);
    return ret;
}
}
