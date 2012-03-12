/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdarg>
#include <cctype>

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#include "os_sep.h"
#include "slang_rs_context.h"
#include "slang_rs_export_var.h"
#include "slang_rs_export_foreach.h"
#include "slang_rs_export_func.h"
#include "slang_rs_reflect_utils.h"
#include "slang_version.h"
#include "slang_utils.h"

#include "slang_rs_reflection_cpp.h"

using namespace std;

namespace slang {

RSReflectionCpp::RSReflectionCpp(const RSContext *con) :
    RSReflectionBase(con) {

}

RSReflectionCpp::~RSReflectionCpp() {

}

bool RSReflectionCpp::reflect(const string &OutputPathBase,
                              const string &InputFileName,
                              const string &OutputBCFileName) {
    mInputFileName = InputFileName;
    mOutputPath = OutputPathBase;
    mOutputBCFileName = OutputBCFileName;
    mClassName = string("ScriptC_") + stripRS(InputFileName);

    makeHeader("ScriptC");
    std::vector< std::string > header(mText);
    mText.clear();

    makeImpl("ScriptC");
    std::vector< std::string > cpp(mText);
    mText.clear();


    RSReflectionBase::writeFile(mClassName + ".h", header);
    RSReflectionBase::writeFile(mClassName + ".cpp", cpp);


    return false;
}

typedef std::vector<std::pair<std::string, std::string> > ArgTy;


#define RS_TYPE_CLASS_NAME_PREFIX        "ScriptField_"



bool RSReflectionCpp::makeHeader(const std::string &baseClass) {
    startFile(mClassName + ".h");

    write("");
    write("#include \"ScriptC.h\"");
    write("");

    // Imports
    //for(unsigned i = 0; i < (sizeof(Import) / sizeof(const char*)); i++)
        //out() << "import " << Import[i] << ";" << std::endl;
    //out() << std::endl;

    if(!baseClass.empty()) {
        write("class " + mClassName + " : public " + baseClass + " {");
    } else {
        write("class " + mClassName + " {");
    }

    write("public:");
    incIndent();
    write(mClassName + "(RenderScript *rs, const char *cacheDir, size_t cacheDirLength);");
    write("virtual ~" + mClassName + "();");


    // Reflect export variable
    uint32_t slot = 0;
    for (RSContext::const_export_var_iterator I = mRSContext->export_vars_begin(),
           E = mRSContext->export_vars_end(); I != E; I++) {

        const RSExportVar *ev = *I;
        //const RSExportType *et = ev->getType();

        char buf[256];
        sprintf(buf, " = %i;", slot++);
        write("const static int mExportVarIdx_" + ev->getName() + buf);

        //switch (ET->getClass()) {

        //genExportVariable(C, *I);
    }

    // Reflect export for each functions
    for (RSContext::const_export_foreach_iterator I = mRSContext->export_foreach_begin(),
             E = mRSContext->export_foreach_end(); I != E; I++, slot++) {

        const RSExportForEach *ef = *I;
        if (ef->isDummyRoot()) {
            write("// No forEach_root(...)");
            continue;
        }

        char buf[256];
        sprintf(buf, " = %i;", slot);
        write("const static int mExportForEachIdx_" + ef->getName() + buf);

        string tmp("void forEach_" + ef->getName() + "(");
        if(ef->hasIn())
            tmp += "Allocation ain";
        if(ef->hasOut())
            tmp += "Allocation aout";

        if(ef->getParamPacketType()) {
            for(RSExportForEach::const_param_iterator i = ef->params_begin(),
                     e = ef->params_end(); i != e; i++) {

                RSReflectionTypeData rtd;
                (*i)->getType()->convertToRTD(&rtd);
                tmp += rtd.type->c_name;
                tmp += " ";
                tmp +=(*i)->getName();
            }
        }
        tmp += ") const;";
        write(tmp);
    }


    // Reflect export function
    for (RSContext::const_export_func_iterator I = mRSContext->export_funcs_begin(),
           E = mRSContext->export_funcs_end(); I != E; I++) {

        //genExportFunction(C, *I);
    }

    decIndent();
    write("};");
    return true;
}

bool RSReflectionCpp::writeBC() {
    FILE *pfin = fopen(mOutputBCFileName.c_str(), "rb");
    if (pfin == NULL) {
        fprintf(stderr, "Error: could not read file %s\n", mOutputBCFileName.c_str());
        return false;
    }

    unsigned char buf[16];
    int read_length;
    write("static const unsigned char __txt[] = {");
    incIndent();
    while ((read_length = fread(buf, 1, sizeof(buf), pfin)) > 0) {
        string s;
        for(int i = 0; i < read_length; i++) {
            char buf2[16];
            sprintf(buf2, "0x%02x,", buf[i]);
            s += buf2;
        }
        write(s);
    }
    decIndent();
    write("};");
    write("");
    return true;
}

bool RSReflectionCpp::makeImpl(const std::string &baseClass) {
    startFile(mClassName + ".h");

    write("");
    write("#include \"" + mClassName + ".h\"");
    write("");

    writeBC();

    // Imports
    //for(unsigned i = 0; i < (sizeof(Import) / sizeof(const char*)); i++)
        //out() << "import " << Import[i] << ";" << std::endl;
    //out() << std::endl;

    write(mClassName + "::" + mClassName +
          "(RenderScript *rs, const char *cacheDir, size_t cacheDirLength) :");
    write("        ScriptC(rs, __txt, sizeof(__txt), " + mInputFileName +
          ", 4, cacheDir, cacheDirLength) {");
    incIndent();
    //...
    decIndent();
    write("}");
    write("");

    write("virtual ~" + mClassName + "::" + mClassName + "() {");
    write("}");
    write("");


    // Reflect export variable
    uint32_t slot = 0;
    for (RSContext::const_export_var_iterator I = mRSContext->export_vars_begin(),
           E = mRSContext->export_vars_end(); I != E; I++) {

        const RSExportVar *ev = *I;
        //const RSExportType *et = ev->getType();

        char buf[256];
        sprintf(buf, " = %i;", slot++);
        write("const static int mExportVarIdx_" + ev->getName() + buf);

        //switch (ET->getClass()) {

        //genExportVariable(C, *I);
    }

    // Reflect export for each functions
    slot = 0;
    for (RSContext::const_export_foreach_iterator I = mRSContext->export_foreach_begin(),
             E = mRSContext->export_foreach_end(); I != E; I++, slot++) {

        const RSExportForEach *ef = *I;
        if (ef->isDummyRoot()) {
            write("// No forEach_root(...)");
            continue;
        }

        char buf[256];
        string tmp("void forEach_" + ef->getName() + "(");
        if(ef->hasIn() && ef->hasOut()) {
            tmp += "Allocation ain, Allocation aout";
        } else if(ef->hasIn()) {
            tmp += "Allocation ain";
        } else {
            tmp += "Allocation aout";
        }
        tmp += ") const {";
        write(tmp);

        incIndent();
        sprintf(buf, "forEach(%i, ", slot);
        tmp = buf;
        if(ef->hasIn() && ef->hasOut()) {
            tmp += "ain, aout, NULL, 0);";
        } else if(ef->hasIn()) {
            tmp += "ain, NULL, 0);";
        } else {
            tmp += "aout, NULL, 0);";
        }
        write(tmp);
        decIndent();

        write("}");
        write("");
    }


    // Reflect export function
    slot = 0;
    for (RSContext::const_export_func_iterator I = mRSContext->export_funcs_begin(),
           E = mRSContext->export_funcs_end(); I != E; I++) {

        //genExportFunction(C, *I);
    }

    decIndent();
    return true;
}



}
