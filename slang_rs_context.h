#ifndef _SLANG_COMPILER_RS_CONTEXT_H
#define _SLANG_COMPILER_RS_CONTEXT_H

#include <map>
#include <list>
#include <string>
#include <cstdio>

#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringMap.h"

#include "clang/Lex/Preprocessor.h"

#include "slang_rs_export_element.h"

namespace llvm {
  class LLVMContext;
  class TargetData;
}   // namespace llvm

namespace clang {
  class VarDecl;
  class ASTContext;
  class TargetInfo;
  class FunctionDecl;
  class SourceManager;
}   // namespace clang

namespace slang {

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
  clang::Preprocessor *mPP;
  clang::ASTContext *mCtx;
  const clang::TargetInfo *mTarget;

  llvm::TargetData *mTargetData;
  llvm::LLVMContext &mLLVMContext;

  // Record the variables/types/elements annotated in #pragma to be exported
  NeedExportVarSet mNeedExportVars;
  NeedExportFuncSet mNeedExportFuncs;
  NeedExportTypeSet mNeedExportTypes;
  bool mExportAllNonStaticVars;
  bool mExportAllNonStaticFuncs;

  std::string *mLicenseNote;
  std::string mReflectJavaPackageName;
  std::string mReflectJavaPathName;

  bool processExportVar(const clang::VarDecl *VD);
  bool processExportFunc(const clang::FunctionDecl *FD);
  bool processExportType(const llvm::StringRef &Name);

  ExportVarList mExportVars;
  ExportFuncList mExportFuncs;
  ExportTypeMap mExportTypes;

 public:
  RSContext(clang::Preprocessor *PP,
            clang::ASTContext *Ctx,
            const clang::TargetInfo *Target);

  inline clang::Preprocessor *getPreprocessor() const { return mPP; }
  inline clang::ASTContext *getASTContext() const { return mCtx; }
  inline const llvm::TargetData *getTargetData() const { return mTargetData; }
  inline llvm::LLVMContext &getLLVMContext() const { return mLLVMContext; }
  inline const clang::SourceManager *getSourceManager() const {
    return &mPP->getSourceManager();
  }

  inline void setLicenseNote(const std::string &S) {
    mLicenseNote = new std::string(S);
  }
  inline const std::string *getLicenseNote() const { return mLicenseNote; }

  inline void addExportVar(const std::string &S) {
    mNeedExportVars.insert(S);
    return;
  }
  inline void addExportFunc(const std::string &S) {
    mNeedExportFuncs.insert(S);
    return;
  }
  inline void addExportType(const std::string &S) {
    mNeedExportTypes.insert(S);
    return;
  }

  inline void setExportAllNonStaticVars(bool flag) {
    mExportAllNonStaticVars = flag;
  }
  inline void setExportAllNonStaticFuncs(bool flag) {
    mExportAllNonStaticFuncs = flag;
  }
  inline void setReflectJavaPackageName(const std::string &S) {
    mReflectJavaPackageName = S;
    return;
  }
  inline void setReflectJavaPathName(const std::string &S) {
    mReflectJavaPathName = S;
    return;
  }
  inline std::string getReflectJavaPathName() const {
    return mReflectJavaPathName;
  }

  void processExport();

  typedef ExportVarList::const_iterator const_export_var_iterator;
  const_export_var_iterator export_vars_begin() const {
    return mExportVars.begin();
  }
  const_export_var_iterator export_vars_end() const {
    return mExportVars.end();
  }
  inline bool hasExportVar() const {
    return !mExportVars.empty();
  }

  typedef ExportFuncList::const_iterator const_export_func_iterator;
  const_export_func_iterator export_funcs_begin() const {
    return mExportFuncs.begin();
  }
  const_export_func_iterator export_funcs_end() const {
    return mExportFuncs.end();
  }
  inline bool hasExportFunc() const { return !mExportFuncs.empty(); }

  typedef ExportTypeMap::iterator export_type_iterator;
  typedef ExportTypeMap::const_iterator const_export_type_iterator;
  export_type_iterator export_types_begin() { return mExportTypes.begin(); }
  export_type_iterator export_types_end() { return mExportTypes.end(); }
  const_export_type_iterator export_types_begin() const {
    return mExportTypes.begin();
  }
  const_export_type_iterator export_types_end() const {
    return mExportTypes.end();
  }
  inline bool hasExportType() const { return !mExportTypes.empty(); }
  export_type_iterator findExportType(const llvm::StringRef &TypeName) {
    return mExportTypes.find(TypeName);
  }
  const_export_type_iterator findExportType(const llvm::StringRef &TypeName)
      const {
    return mExportTypes.find(TypeName);
  }

  // Insert the specified Typename/Type pair into the map. If the key already
  // exists in the map, return false and ignore the request, otherwise insert it
  // and return true.
  bool insertExportType(const llvm::StringRef &TypeName, RSExportType *Type);

  bool reflectToJava(const char *OutputPackageName,
                     const std::string &InputFileName,
                     const std::string &OutputBCFileName,
                     char realPackageName[],
                     int bSize);
  bool reflectToJavaPath(const char *OutputPathName);

  ~RSContext();
};

}   // namespace slang

#endif  // _SLANG_COMPILER_RS_CONTEXT_H
