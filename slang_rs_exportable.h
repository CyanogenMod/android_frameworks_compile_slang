#ifndef _SLANG_COMPILER_RS_EXPORTABLE_HPP
#define _SLANG_COMPILER_RS_EXPORTABLE_HPP

#include "slang_rs_context.h"

namespace slang {

class RSExportable {
public:
  enum Kind {
    EX_FUNC,
    EX_TYPE,
    EX_VAR
  };

private:
  Kind mK;

protected:
  RSExportable(RSContext *Context, RSExportable::Kind K) : mK(K) {
    Context->newExportable(this);
    return;
  }

public:
  inline Kind getKind() const { return mK; }

  virtual ~RSExportable() { }
};

}

#endif  // _SLANG_COMPILER_RS_EXPORTABLE_HPP
