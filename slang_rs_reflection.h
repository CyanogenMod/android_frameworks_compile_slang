/*
 * Copyright 2010-2012, The Android Open Source Project
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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_REFLECTION_H_ // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_REFLECTION_H_

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "llvm/ADT/StringExtras.h"

#include "slang_assert.h"
#include "slang_rs_export_type.h"

namespace slang {

class RSContext;
class RSExportVar;
class RSExportFunc;
class RSExportForEach;

class RSReflectionJava {
private:
  const RSContext *mRSContext;

  std::string mLastError;
  std::vector<std::string> *mGeneratedFileNames;

  inline void setError(const std::string &Error) { mLastError = Error; }

  static const char *const ApacheLicenseNote;

  bool mVerbose;

  std::string mOutputPathBase;

  std::string mInputFileName;

  std::string mPackageName;
  std::string mRSPackageName;
  std::string mResourceId;
  std::string mPaddingPrefix;

  std::string mClassName;

  std::string mLicenseNote;

  bool mEmbedBitcodeInJava;

  std::string mIndent;

  int mPaddingFieldIndex;

  int mNextExportVarSlot;
  int mNextExportFuncSlot;
  int mNextExportForEachSlot;

  // A mapping from a field in a record type to its index in the rsType
  // instance. Only used when generates TypeClass (ScriptField_*).
  typedef std::map<const RSExportRecordType::Field *, unsigned> FieldIndexMapTy;
  FieldIndexMapTy mFieldIndexMap;
  // Field index of current processing TypeClass.
  unsigned mFieldIndex;

  inline void clear() {
    mClassName = "";
    mIndent = "";
    mPaddingFieldIndex = 1;
    mNextExportVarSlot = 0;
    mNextExportFuncSlot = 0;
    mNextExportForEachSlot = 0;
  }

  bool openClassFile(const std::string &ClassName, std::string &ErrorMsg);

public:
  typedef enum {
    AM_Public,
    AM_Protected,
    AM_Private,
    AM_PublicSynchronized
  } AccessModifier;

  mutable std::ofstream mOF;

  // Generated RS Elements for type-checking code.
  std::set<std::string> mTypesToCheck;

  // Generated FieldPackers for unsigned setters/validation.
  std::set<std::string> mFieldPackerTypes;

  bool addTypeNameForElement(const std::string &TypeName);
  bool addTypeNameForFieldPacker(const std::string &TypeName);

  static const char *AccessModifierStr(AccessModifier AM);

  inline std::string &getInputFileName() { return mInputFileName; }

  inline std::ostream &out() const { return mOF; }
  inline std::ostream &indent() const {
    out() << mIndent;
    return out();
  }

  inline void incIndentLevel() { mIndent.append(4, ' '); }

  inline void decIndentLevel() {
    slangAssert(getIndentLevel() > 0 && "No indent");
    mIndent.erase(0, 4);
  }

  inline int getIndentLevel() { return (mIndent.length() >> 2); }

  inline bool getEmbedBitcodeInJava() const { return mEmbedBitcodeInJava; }

  inline int getNextExportVarSlot() { return mNextExportVarSlot++; }

  inline int getNextExportFuncSlot() { return mNextExportFuncSlot++; }
  inline int getNextExportForEachSlot() { return mNextExportForEachSlot++; }

  // Will remove later due to field name information is not necessary for
  // C-reflect-to-Java
  inline std::string createPaddingField() {
    return mPaddingPrefix + llvm::itostr(mPaddingFieldIndex++);
  }

  inline void setLicenseNote(const std::string &LicenseNote) {
    mLicenseNote = LicenseNote;
  }

  bool startClass(AccessModifier AM, bool IsStatic,
                  const std::string &ClassName, const char *SuperClassName,
                  std::string &ErrorMsg);
  void endClass();

  void startFunction(AccessModifier AM, bool IsStatic, const char *ReturnType,
                     const std::string &FunctionName, int Argc, ...);

  typedef std::vector<std::pair<std::string, std::string>> ArgTy;
  void startFunction(AccessModifier AM, bool IsStatic, const char *ReturnType,
                     const std::string &FunctionName, const ArgTy &Args);
  void endFunction();

  void startBlock(bool ShouldIndent = false);
  void endBlock();

  inline const std::string &getPackageName() const { return mPackageName; }
  inline const std::string &getRSPackageName() const { return mRSPackageName; }
  inline const std::string &getClassName() const { return mClassName; }
  inline const std::string &getResourceId() const { return mResourceId; }

  void startTypeClass(const std::string &ClassName);
  void endTypeClass();

  inline void incFieldIndex() { mFieldIndex++; }

  inline void resetFieldIndex() { mFieldIndex = 0; }

  inline void addFieldIndexMapping(const RSExportRecordType::Field *F) {
    slangAssert((mFieldIndexMap.find(F) == mFieldIndexMap.end()) &&
                "Nested structure never occurs in C language.");
    mFieldIndexMap.insert(std::make_pair(F, mFieldIndex));
  }

  inline unsigned getFieldIndex(const RSExportRecordType::Field *F) const {
    FieldIndexMapTy::const_iterator I = mFieldIndexMap.find(F);
    slangAssert((I != mFieldIndexMap.end()) &&
                "Requesting field is out of scope.");
    return I->second;
  }

  inline void clearFieldIndexMap() { mFieldIndexMap.clear(); }

private:
  bool genScriptClass(const std::string &ClassName, std::string &ErrorMsg);
  void genScriptClassConstructor();

  void genInitBoolExportVariable(const std::string &VarName,
                                 const clang::APValue &Val);
  void genInitPrimitiveExportVariable(const std::string &VarName,
                                      const clang::APValue &Val);
  void genInitExportVariable(const RSExportType *ET, const std::string &VarName,
                             const clang::APValue &Val);
  void genExportVariable(const RSExportVar *EV);
  void genPrimitiveTypeExportVariable(const RSExportVar *EV);
  void genPointerTypeExportVariable(const RSExportVar *EV);
  void genVectorTypeExportVariable(const RSExportVar *EV);
  void genMatrixTypeExportVariable(const RSExportVar *EV);
  void genConstantArrayTypeExportVariable(const RSExportVar *EV);
  void genRecordTypeExportVariable(const RSExportVar *EV);
  void genPrivateExportVariable(const std::string &TypeName,
                                const std::string &VarName);
  void genSetExportVariable(const std::string &TypeName, const RSExportVar *EV);
  void genGetExportVariable(const std::string &TypeName,
                            const std::string &VarName);
  void genGetFieldID(const std::string &VarName);

  void genExportFunction(const RSExportFunc *EF);

  void genExportForEach(const RSExportForEach *EF);

  void genTypeCheck(const RSExportType *ET, const char *VarName);

  void genTypeInstanceFromPointer(const RSExportType *ET);

  void genTypeInstance(const RSExportType *ET);

  void genFieldPackerInstance(const RSExportType *ET);

  bool genTypeClass(const RSExportRecordType *ERT, std::string &ErrorMsg);
  void genTypeItemClass(const RSExportRecordType *ERT);
  void genTypeClassConstructor(const RSExportRecordType *ERT);
  void genTypeClassCopyToArray(const RSExportRecordType *ERT);
  void genTypeClassCopyToArrayLocal(const RSExportRecordType *ERT);
  void genTypeClassItemSetter(const RSExportRecordType *ERT);
  void genTypeClassItemGetter(const RSExportRecordType *ERT);
  void genTypeClassComponentSetter(const RSExportRecordType *ERT);
  void genTypeClassComponentGetter(const RSExportRecordType *ERT);
  void genTypeClassCopyAll(const RSExportRecordType *ERT);
  void genTypeClassResize();

  void genBuildElement(const char *ElementBuilderName,
                       const RSExportRecordType *ERT,
                       const char *RenderScriptVar, bool IsInline);
  void genAddElementToElementBuilder(const RSExportType *ERT,
                                     const std::string &VarName,
                                     const char *ElementBuilderName,
                                     const char *RenderScriptVar,
                                     unsigned ArraySize);
  void genAddPaddingToElementBuilder(int PaddingSize,
                                     const char *ElementBuilderName,
                                     const char *RenderScriptVar);

  bool genCreateFieldPacker(const RSExportType *T, const char *FieldPackerName);
  void genPackVarOfType(const RSExportType *T, const char *VarName,
                        const char *FieldPackerName);
  void genAllocateVarOfType(const RSExportType *T, const std::string &VarName);
  void genNewItemBufferIfNull(const char *Index);
  void genNewItemBufferPackerIfNull();

public:
  explicit RSReflectionJava(const RSContext *Context,
                            std::vector<std::string> *GeneratedFileNames)
      : mRSContext(Context), mLastError(""),
        mGeneratedFileNames(GeneratedFileNames) {
    slangAssert(mGeneratedFileNames && "Must supply GeneratedFileNames");
  }

  bool reflect(const std::string &OutputPathBase,
               const std::string &OutputPackageName,
               const std::string &RSPackageName,
               const std::string &InputFileName,
               const std::string &OutputBCFileName, bool EmbedBitcodeInJava);

  inline const char *getLastError() const {
    if (mLastError.empty())
      return NULL;
    else
      return mLastError.c_str();
  }
}; // class RSReflectionJava

} // namespace slang

#endif // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_REFLECTION_H_  NOLINT
