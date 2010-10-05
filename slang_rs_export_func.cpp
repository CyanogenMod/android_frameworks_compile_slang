#include "slang_rs_export_func.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Target/TargetData.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"

#include "slang_rs_context.h"

using namespace slang;

RSExportFunc *RSExportFunc::Create(RSContext *Context,
                                   const clang::FunctionDecl *FD) {
  llvm::StringRef Name = FD->getName();
  RSExportFunc *F;

  assert(!Name.empty() && "Function must have a name");

  F = new RSExportFunc(Context, Name);

  // Initialize mParamPacketType
  if (FD->getNumParams() <= 0) {
    F->mParamPacketType = NULL;
  } else {
    clang::ASTContext *Ctx = Context->getASTContext();

    std::string Id(DUMMY_RS_TYPE_NAME_PREFIX"helper_func_param:");
    Id.append(F->getName()).append(DUMMY_RS_TYPE_NAME_POSTFIX);

    clang::RecordDecl *RD =
        clang::RecordDecl::Create(*Ctx, clang::TTK_Struct,
                                  Ctx->getTranslationUnitDecl(),
                                  clang::SourceLocation(),
                                  &Ctx->Idents.get(Id));

    for (unsigned i = 0; i < FD->getNumParams(); i++) {
      const clang::ParmVarDecl *PVD = FD->getParamDecl(i);
      llvm::StringRef ParamName = PVD->getName();

      if (PVD->hasDefaultArg())
        fprintf(stderr, "Note: parameter '%s' in function '%s' has default "
                        "value which is not supported\n",
                        ParamName.str().c_str(),
                        F->getName().c_str());

      clang::FieldDecl *FD =
          clang::FieldDecl::Create(*Ctx,
                                   RD,
                                   clang::SourceLocation(),
                                   PVD->getIdentifier(),
                                   PVD->getOriginalType(),
                                   NULL,
                                   /* BitWidth = */NULL,
                                   /* Mutable = */false);
      RD->addDecl(FD);
    }

    RD->completeDefinition();

    clang::QualType T = Ctx->getTagDeclType(RD);
    assert(!T.isNull());

    RSExportType *ET =
      RSExportType::Create(Context, T.getTypePtr());

    if (ET == NULL) {
      fprintf(stderr, "Failed to export the function %s. There's at least one  "
                      "parameter whose type is not supported by the "
                      "reflection", F->getName().c_str());
      delete F;
      return NULL;
    }

    assert((ET->getClass() == RSExportType::ExportClassRecord) &&
           "Parameter packet must be a record");

    F->mParamPacketType = static_cast<RSExportRecordType *>(ET);
  }

  return F;
}

bool
RSExportFunc::checkParameterPacketType(const llvm::StructType *ParamTy) const {
  if (ParamTy == NULL)
    return !hasParam();
  else if (!hasParam())
    return false;

  assert(mParamPacketType != NULL);

  const RSExportRecordType *ERT = mParamPacketType;
  // must have same number of elements
  if (ERT->getFields().size() != ParamTy->getNumElements())
    return false;

  const llvm::StructLayout *ParamTySL =
      mContext->getTargetData()->getStructLayout(ParamTy);

  unsigned Index = 0;
  for (RSExportRecordType::const_field_iterator FI = ERT->fields_begin(),
       FE = ERT->fields_end(); FI != FE; FI++, Index++) {
    const RSExportRecordType::Field *F = *FI;

    const llvm::Type *T1 = F->getType()->getLLVMType();
    const llvm::Type *T2 = ParamTy->getTypeAtIndex(Index);

    // Fast check
    if (T1 == T2)
      continue;

    // Check offset
    size_t T1Offset = F->getOffsetInParent();
    size_t T2Offset = ParamTySL->getElementOffset(Index);

    if (T1Offset != T2Offset)
      return false;

    // Check size
    size_t T1Size = RSExportType::GetTypeAllocSize(F->getType());
    size_t T2Size = mContext->getTargetData()->getTypeAllocSize(T2);

    if (T1Size != T2Size)
      return false;
  }

  return true;
}
