#ifndef _SLANG_COMPILER_RS_EXPORT_ELEMENT_HPP
#   define _SLANG_COMPILER_RS_EXPORT_ELEMENT_HPP

#include "slang_rs_export_type.hpp"

#include "llvm/ADT/StringMap.h"     /* for class llvm::StringMap */
#include "llvm/ADT/StringRef.h"     /* for class llvm::StringRef */

#include "clang/Lex/Token.h"        /* for class clang::Token */

#include <string>

namespace clang {
class Type;
class DeclaratorDecl;
}   // namespace clang

namespace slang {

class RSContext;
class RSExportType;

class RSExportElement {
  // This is a utility class for handling the RS_ELEMENT_ADD* marker
  RSExportElement() { return; }

  typedef struct {
    RSExportPrimitiveType::DataKind kind;
    RSExportPrimitiveType::DataType type;
    bool normalized;
    int vsize;
  } ElementInfo;

  typedef llvm::StringMap<const ElementInfo*> ElementInfoMapTy;

 private:
  // Macro name <-> ElementInfo
  static ElementInfoMapTy ElementInfoMap;

  static bool Initialized;

  static RSExportType *Create(RSContext *Context,
                              const clang::Type* T,
                              const ElementInfo *EI);

  static const ElementInfo *GetElementInfo(const llvm::StringRef &Name);
 public:
  static void Init();

  static RSExportType *CreateFromDecl(RSContext* Context,
                                      const clang::DeclaratorDecl* DD);
};

}   // namespace slang

#endif  // _SLANG_COMPILER_RS_EXPORT_ELEMENT_HPP
