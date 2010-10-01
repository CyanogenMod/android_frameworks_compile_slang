#ifndef _SLANG_COMPILER_RS_EXPORT_FUNC_HPP
#   define _SLANG_COMPILER_RS_EXPORT_FUNC_HPP

#include "llvm/ADT/StringRef.h"

#include <list>
#include <string>

namespace clang {
class FunctionDecl;
}   // namespace clang

namespace slang {

class RSContext;
class RSExportType;
class RSExportRecordType;

class RSExportFunc {
  friend class RSContext;
 public:
  class Parameter {
   private:
    RSExportType *mType;
    std::string mName;

   public:
    Parameter(RSExportType *T, const llvm::StringRef &Name) :
        mType(T),
        mName(Name.data(), Name.size())
    {
      return;
    }

    inline const RSExportType *getType() const { return mType; }
    inline const std::string &getName() const { return mName; }
  };

 private:
  RSContext *mContext;
  std::string mName;
  std::list<const Parameter*> mParams;
  mutable RSExportRecordType *mParamPacketType;

  RSExportFunc(RSContext *Context, const llvm::StringRef &Name) :
      mContext(Context),
      mName(Name.data(), Name.size()),
      mParamPacketType(NULL)
  {
    return;
  }

 public:
  static RSExportFunc *Create(RSContext *Context,
                              const clang::FunctionDecl *FD);

  typedef std::list<const Parameter*>::const_iterator const_param_iterator;

  inline const_param_iterator params_begin() const {
    return this->mParams.begin();
  }
  inline const_param_iterator params_end() const {
    return this->mParams.end();
  }

  inline const std::string &getName() const {
    return mName;
  }
  inline RSContext *getRSContext() const {
    return mContext;
  }

  inline bool hasParam() const {
    return !mParams.empty();
  }
  inline int getNumParameters() const {
    return mParams.size();
  }

  const RSExportRecordType *getParamPacketType() const;

  ~RSExportFunc();

};  // RSExportFunc


}   // namespace slang

#endif  // _SLANG_COMPILER_RS_EXPORT_FUNC_HPP
