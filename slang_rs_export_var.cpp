#include "slang_rs_context.hpp"
#include "slang_rs_export_var.hpp"
#include "slang_rs_export_type.hpp" /* for macro GET_CANONICAL_TYPE() */

#include "clang/AST/Type.h"         /* for class clang::Type and clang::QualType */

#include "llvm/ADT/APSInt.h"        /* for class llvm::APSInt */

namespace slang {

RSExportVar::RSExportVar(RSContext* Context, const VarDecl* VD, const RSExportType* ET) :
    mContext(Context),
    mName(VD->getName().data(), VD->getName().size()),
    mET(ET)
{
  const Expr* Initializer = VD->getAnyInitializer();
  if(Initializer != NULL)
    switch(ET->getClass()) {
      case RSExportType::ExportClassPrimitive:
      case RSExportType::ExportClassVector:
        Initializer->EvaluateAsAny(mInit, *Context->getASTContext());
        break;

      case RSExportType::ExportClassPointer:
        if(Initializer->isNullPointerConstant(*Context->getASTContext(), Expr::NPC_ValueDependentIsNotNull))
          mInit.Val = APValue(llvm::APSInt(1));
        else
          Initializer->EvaluateAsAny(mInit, *Context->getASTContext());
        break;

      case RSExportType::ExportClassRecord:
        /* No action */
        printf("RSExportVar::RSExportVar : Reflection of initializer to variable '%s' (of type '%s') is unsupported currently.\n", mName.c_str(), ET->getName().c_str());
        break;

      default:
        assert(false && "Unknown class of type");
        break;
    }

  return;
}

}   /* namespace slang */
