#ifndef _SLANG_COMPILER_RS_REFLECTION_HPP
#   define _SLANG_COMPILER_RS_REFLECTION_HPP

#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <iostream>

#include "slang_rs_export_type.hpp"

#include "llvm/ADT/StringExtras.h"  /* for function llvm::utostr_32() and llvm::itostr() */

namespace slang {

class RSContext;
class RSExportVar;
class RSExportFunc;
class RSExportRecordType;

class RSReflection {
private:
    const RSContext* mRSContext;

    std::string mLastError;
    inline void setError(const std::string& Error) { mLastError = Error; }

    class Context {
    private:
        static const char* const LicenseNote;
        static const char* const Import[];

        bool mUseStdout;
        mutable std::ofstream mOF;

        bool mVerbose;

        std::string mPackageName;
        std::string mResourceId;

        std::string mClassName;

        std::string mIndent;

        int mPaddingFieldIndex;

        int mNextExportVarSlot;
        int mNextExportFuncSlot;

        inline void clear() {
            mClassName = "";
            mIndent = "";
            mPaddingFieldIndex = 1;
            mNextExportVarSlot = 0;
            mNextExportFuncSlot = 0;
            return;
        }

    public:
        typedef enum {
            AM_Public,
            AM_Protected,
            AM_Private
        } AccessModifier;

        static const char* AccessModifierStr(AccessModifier AM);

        Context(const std::string& PackageName, const std::string& ResourceId, bool UseStdout) :
            mPackageName(PackageName),
            mResourceId(ResourceId),
            mUseStdout(UseStdout),
            mVerbose(true)
        {
            clear();
            return;
        }

        inline std::ostream& out() const { if(mUseStdout) return std::cout; else return mOF; }
        inline std::ostream& indent() const {
            out() << mIndent;
            return out();
        }

        inline void incIndentLevel() {
            mIndent.append(4, ' ');
            return;
        }

        inline void decIndentLevel() {
            assert(getIndentLevel() > 0 && "No indent");
            mIndent.erase(0, 4);
            return;
        }

        inline int getIndentLevel() {
            return (mIndent.length() >> 2);
        }

        inline int getNextExportVarSlot() {
            return mNextExportVarSlot++;
        }

        inline int getNextExportFuncSlot() {
            return mNextExportFuncSlot++;
        }

        /* Will remove later due to field name information is not necessary for C-reflect-to-Java */
        inline std::string createPaddingField() {
            return "#padding_" + llvm::itostr(mPaddingFieldIndex++);
        }

        bool startClass(AccessModifier AM, bool IsStatic, const std::string& ClassName, const char* SuperClassName, std::string& ErrorMsg);
        void endClass();

        void startFunction(AccessModifier AM, bool IsStatic, const char* ReturnType, const std::string& FunctionName, int Argc, ...);

        typedef std::vector<std::pair<std::string, std::string> > ArgTy;
        void startFunction(AccessModifier AM, bool IsStatic, const char* ReturnType, const std::string& FunctionName, const ArgTy& Args);
        void endFunction();

        void startBlock(bool ShouldIndent = false);
        void endBlock();

        inline const std::string& getPackageName() const { return mPackageName; }
        inline const std::string& getClassName() const { return mClassName; }
        inline const std::string& getResourceId() const { return mResourceId; }

        void startTypeClass(const std::string& ClassName);
        void endTypeClass();
    };

    bool genScriptClass(Context& C, const std::string& ClassName, std::string& ErrorMsg);
    void genScriptClassConstructor(Context& C);

    void genExportVariable(Context& C, const RSExportVar* EV);
    void genPrimitiveTypeExportVariable(Context& C, const RSExportVar* EV);
    void genPointerTypeExportVariable(Context& C, const RSExportVar* EV);
    void genVectorTypeExportVariable(Context& C, const RSExportVar* EV);
    void genRecordTypeExportVariable(Context& C, const RSExportVar* EV);
    void genGetExportVariable(Context& C, const std::string& TypeName, const std::string& VarName);

    void genExportFunction(Context& C, const RSExportFunc* EF);

    bool genTypeClass(Context& C, const RSExportRecordType* ERT, std::string& ErrorMsg);
    bool genTypeItemClass(Context& C, const RSExportRecordType* ERT, std::string& ErrorMsg);
    void genTypeClassConstructor(Context& C, const RSExportRecordType* ERT);
    void genTypeClassCopyToArray(Context& C, const RSExportRecordType* ERT);
    void genTypeClasSet(Context& C, const RSExportRecordType* ERT);
    void genTypeClasCopyAll(Context& C, const RSExportRecordType* ERT);

    void genBuildElement(Context& C, const RSExportRecordType* ERT, const char* RenderScriptVar);
    void genAddElementToElementBuilder(Context& C, const RSExportType* ERT, const std::string& VarName, const char* ElementBuilderName, const char* RenderScriptVar);
    void genAddPaddingToElementBuiler(Context& C, size_t PaddingSize, const char* ElementBuilderName, const char* RenderScriptVar);

    bool genCreateFieldPacker(Context& C, const RSExportType* T, const char* FieldPackerName);
    void genPackVarOfType(Context& C, const RSExportType* T, const char* VarName, const char* FieldPackerName);

public:
    RSReflection(const RSContext* Context) :
        mRSContext(Context),
        mLastError("")
    {
        return;
    }

    bool reflect(const char* OutputPackageName, const std::string& InputFileName, const std::string& OutputBCFileName);

    inline const char* getLastError() const {
        if(mLastError.empty())
            return NULL;
        else
            return mLastError.c_str();
    }
};  /* class RSReflection */

}   /* namespace slang */

#endif  /* _SLANG_COMPILER_RS_REFLECTION_HPP */
