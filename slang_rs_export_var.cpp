#include "slang_rs_export_var.hpp"
#include "slang_rs_export_type.hpp" /* for macro GET_CANONICAL_TYPE() */

#include "clang/AST/Type.h"         /* for clang::Type and clang::QualType */

namespace slang {

RSExportVar::RSExportVar(RSContext* Context, const VarDecl* VD, const RSExportType* ET) :
    mContext(Context),
    mName(VD->getName().data(), VD->getName().size()),
    mET(ET)
{
    return; 
}

}   /* namespace slang */
