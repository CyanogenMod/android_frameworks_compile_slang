#ifndef _SLANG_COMPILER_RS_CONTEXT_HPP
#   define _SLANG_COMPILER_RS_CONTEXT_HPP

#include "slang_rs_export_element.hpp"

#include <map>
#include <list>
#include <string>
#include <cstdio>

#include "llvm/ADT/StringSet.h"     /* for class llvm::StringSet */
#include "llvm/ADT/StringMap.h"     /* for class llvm::StringMap */

#include "clang/Lex/Preprocessor.h"     /* for class clang::Preprocessor */

namespace llvm {
    class LLVMContext;
    class TargetData;
}   /* namespace llvm */

namespace clang {

class VarDecl;
class ASTContext;
class TargetInfo;
class FunctionDecl;
class SourceManager;
    
}   /* namespace clang */

namespace slang {

using namespace clang;

class RSExportVar;
class RSExportFunc;
class RSExportType;
class RSPragmaHandler;

class RSContext {
    typedef llvm::StringSet<> NeedExportVarSet;
    typedef llvm::StringSet<> NeedExportFuncSet;
    typedef llvm::StringSet<> NeedExportTypeSet;

public:
    typedef std::list<RSExportVar*> ExportVarList;
    typedef std::list<RSExportFunc*> ExportFuncList;
    typedef llvm::StringMap<RSExportType*> ExportTypeMap;

private:
    Preprocessor* mPP;
    const TargetInfo* mTarget;

    llvm::TargetData* mTargetData;
    llvm::LLVMContext& mLLVMContext;

    RSPragmaHandler* mRSExportVarPragma;
    RSPragmaHandler* mRSExportFuncPragma;
    RSPragmaHandler* mRSExportTypePragma;
    RSPragmaHandler* mRSJavaPackageNamePragma;

    /* Remember the variables/types/elements annotated in #pragma to be exported */
    NeedExportVarSet mNeedExportVars;
    NeedExportFuncSet mNeedExportFuncs;
    NeedExportTypeSet mNeedExportTypes;

    std::string mReflectJavaPackageName;

    bool processExportVar(const VarDecl* VD);
    bool processExportFunc(const FunctionDecl* FD);
    bool processExportType(ASTContext& Ctx, const llvm::StringRef& Name);

    ExportVarList mExportVars;
    ExportFuncList mExportFuncs;
    ExportTypeMap mExportTypes;

public:
    RSContext(Preprocessor* PP, const TargetInfo* Target);

    inline Preprocessor* getPreprocessor() const { return mPP; }
    inline const llvm::TargetData* getTargetData() const { return mTargetData; }
    inline llvm::LLVMContext& getLLVMContext() const { return mLLVMContext; }
    inline const SourceManager* getSourceManager() const { return &mPP->getSourceManager(); }

    inline void addExportVar(const std::string& S) { mNeedExportVars.insert(S); return; }
    inline void addExportFunc(const std::string& S) { mNeedExportFuncs.insert(S); return; }
    inline void addExportType(const std::string& S) { mNeedExportTypes.insert(S); return; }

    inline void setReflectJavaPackageName(const std::string& S) { mReflectJavaPackageName = S; return; }

    void processExport(ASTContext& Ctx);

    typedef ExportVarList::const_iterator const_export_var_iterator;
    const_export_var_iterator export_vars_begin() const { return mExportVars.begin(); }
    const_export_var_iterator export_vars_end() const { return mExportVars.end(); }
    inline bool hasExportVar() const { return !mExportVars.empty(); }

    typedef ExportFuncList::const_iterator const_export_func_iterator;
    const_export_func_iterator export_funcs_begin() const { return mExportFuncs.begin(); }
    const_export_func_iterator export_funcs_end() const { return mExportFuncs.end(); }
    inline bool hasExportFunc() const { return !mExportFuncs.empty(); }

    typedef ExportTypeMap::iterator export_type_iterator;
    typedef ExportTypeMap::const_iterator const_export_type_iterator;
    export_type_iterator export_types_begin() { return mExportTypes.begin(); }
    export_type_iterator export_types_end() { return mExportTypes.end(); }
    const_export_type_iterator export_types_begin() const { return mExportTypes.begin(); }
    const_export_type_iterator export_types_end() const { return mExportTypes.end(); }
    inline bool hasExportType() const { return !mExportTypes.empty(); }
    export_type_iterator findExportType(const llvm::StringRef& TypeName) { return mExportTypes.find(TypeName); }
    const_export_type_iterator findExportType(const llvm::StringRef& TypeName) const { return mExportTypes.find(TypeName); }
    /* 
     * Insert the specified Typename/Type pair into the map. If the key
     *  already exists in the map, return false and ignore the request, otherwise
     *  insert it and return true.
     */
    bool insertExportType(const llvm::StringRef& TypeName, RSExportType* Type);

    bool reflectToJava(const char* OutputPackageName, const std::string& InputFileName, const std::string& OutputBCFileName);

    ~RSContext();
};

}   /* namespace slang */

#endif  /* _SLANG_COMPILER_RS_CONTEXT_HPP */
