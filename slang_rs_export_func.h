#ifndef _SLANG_COMPILER_RS_EXPORT_FUNC_H
#define _SLANG_COMPILER_RS_EXPORT_FUNC_H

#include <list>
#include <string>

#include "llvm/ADT/StringRef.h"

#include "slang_rs_exportable.h"
#include "slang_rs_export_type.h"

namespace llvm {
  class StructType;
}

namespace clang {
  class FunctionDecl;
}   // namespace clang

namespace slang {

class RSContext;

class RSExportFunc : public RSExportable {
  friend class RSContext;

 private:
  RSContext *mContext;
  std::string mName;
  RSExportRecordType *mParamPacketType;

  RSExportFunc(RSContext *Context, const llvm::StringRef &Name)
    : RSExportable(Context, RSExportable::EX_FUNC),
      mContext(Context),
      mName(Name.data(), Name.size()),
      mParamPacketType(NULL) {
    return;
  }

 public:
  static RSExportFunc *Create(RSContext *Context,
                              const clang::FunctionDecl *FD);

  typedef RSExportRecordType::const_field_iterator const_param_iterator;

  inline const_param_iterator params_begin() const {
    assert((mParamPacketType != NULL) &&
           "Get parameter from export function without parameter!");
    return mParamPacketType->fields_begin();
  }
  inline const_param_iterator params_end() const {
    assert((mParamPacketType != NULL) &&
           "Get parameter from export function without parameter!");
    return mParamPacketType->fields_end();
  }

  inline const std::string &getName() const { return mName; }
  inline RSContext *getRSContext() const { return mContext; }

  inline bool hasParam() const
    { return (mParamPacketType && !mParamPacketType->getFields().empty()); }
  inline size_t getNumParameters() const
    { return ((mParamPacketType) ? mParamPacketType->getFields().size() : 0); }

  inline const RSExportRecordType *getParamPacketType() const
    { return mParamPacketType; }

  // Check whether the given ParamsPacket type (in LLVM type) is "size
  // equivalent" to the one obtained from getParamPacketType(). If the @Params
  // is NULL, means there must be no any parameters.
  bool checkParameterPacketType(const llvm::StructType *ParamTy) const;
};  // RSExportFunc


}   // namespace slang

#endif  // _SLANG_COMPILER_RS_EXPORT_FUNC_H
