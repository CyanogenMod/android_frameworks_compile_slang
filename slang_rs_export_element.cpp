#include "slang_rs_context.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_export_element.hpp"

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/IdentifierTable.h"

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

using namespace slang;

bool RSExportElement::Initialized = false;
RSExportElement::ElementInfoMapTy RSExportElement::ElementInfoMap;

void RSExportElement::Init() {
  if (!Initialized) {
    // Initialize ElementInfoMap
#define USE_ELEMENT_DATA_TYPE
#define USE_ELEMENT_DATA_KIND
#define DEF_ELEMENT(_name, _dk, _dt, _norm, _vsize)     \
    {                                                   \
      ElementInfo *EI = new ElementInfo;                \
      EI->kind = GET_ELEMENT_DATA_KIND(_dk);            \
      EI->type = GET_ELEMENT_DATA_TYPE(_dt);            \
      EI->normalized = _norm;                           \
      EI->vsize = _vsize;                               \
                                                        \
      llvm::StringRef Name(_name);                      \
      ElementInfoMap.insert(                            \
          ElementInfoMapTy::value_type::Create(         \
              Name.begin(),                             \
              Name.end(),                               \
              ElementInfoMap.getAllocator(),            \
              EI                                        \
              ));                                       \
    }
#include "slang_rs_export_element_support.inc"

    Initialized = true;
  }
  return;
}

RSExportType *RSExportElement::Create(RSContext *Context,
                                      const clang::Type *T,
                                      const ElementInfo *EI) {
  // Create RSExportType corresponded to the @T first and then verify

  llvm::StringRef TypeName;
  RSExportType *ET = NULL;

  if (!Initialized)
    Init();

  assert(EI != NULL && "Element info not found");

  if (!RSExportType::NormalizeType(T, TypeName))
    return NULL;

  switch(T->getTypeClass()) {
    case clang::Type::Builtin:
    case clang::Type::Pointer: {
      assert(EI->vsize == 1 && "Element not a primitive class (please check "
                               "your macro)");
      RSExportPrimitiveType *EPT =
          RSExportPrimitiveType::Create(Context,
                                        T,
                                        TypeName,
                                        EI->kind,
                                        EI->normalized);
      // Verify
      assert(EI->type == EPT->getType() && "Element has unexpected type");
      ET = EPT;
      break;
    }

    case clang::Type::ConstantArray: {
      //XXX
      break;
    }

    case clang::Type::ExtVector: {
      assert(EI->vsize > 1 && "Element not a vector class (please check your "
                              "macro)");
      RSExportVectorType *EVT =
          RSExportVectorType::Create(Context,
                                     static_cast<clang::ExtVectorType*>(
                                         T->getCanonicalTypeInternal()
                                             .getTypePtr()),
                                     TypeName,
                                     EI->kind,
                                     EI->normalized);
      // Verify
      assert(EI->type == EVT->getType() && "Element has unexpected type");
      assert(EI->vsize == EVT->getNumElement() && "Element has unexpected size "
                                                  "of vector");
      ET = EVT;
      break;
    }

    case clang::Type::Record: {
      // Must be RS object type

      if ( TypeName.equals(llvm::StringRef("rs_matrix2x2")) ||
           TypeName.equals(llvm::StringRef("rs_matrix3x3")) ||
           TypeName.equals(llvm::StringRef("rs_matrix4x4")) ) {

        const clang::RecordType *RT = static_cast<const clang::RecordType*> (T);
        const clang::RecordDecl *RD = RT->getDecl();
        RD = RD->getDefinition();
        clang::RecordDecl::field_iterator fit = RD->field_begin();
        clang::FieldDecl *FD = *fit;
        const clang::Type *FT = RSExportType::GetTypeOfDecl(FD);
        RSExportConstantArrayType *ECT =
            RSExportConstantArrayType::Create(
                Context,
                static_cast<const clang::ConstantArrayType*> (FT),
                TypeName
                                              );
        ET = ECT;
      } else {
        RSExportPrimitiveType* EPT =
            RSExportPrimitiveType::Create(Context,
                                          T,
                                          TypeName,
                                          EI->kind,
                                          EI->normalized);
        // Verify
        assert(EI->type == EPT->getType() && "Element has unexpected type");
        ET = EPT;
      }
      break;
    }

    default: {
      // TODO: warning: type is not exportable
      fprintf(stderr, "RSExportElement::Create : type '%s' is not exportable\n",
              T->getTypeClassName());
      break;
    }
  }

  return ET;
}

RSExportType *RSExportElement::CreateFromDecl(RSContext *Context,
                                              const clang::DeclaratorDecl *DD) {
  const clang::Type* T = RSExportType::GetTypeOfDecl(DD);
  const clang::Type* CT = GET_CANONICAL_TYPE(T);
  const ElementInfo* EI = NULL;

  // Note: RS element like rs_pixel_rgb elements are either in the type of
  // primitive or vector.
  if ((CT->getTypeClass() != clang::Type::Builtin) &&
      (CT->getTypeClass() != clang::Type::ExtVector) &&
      (CT->getTypeClass() != clang::Type::Record)) {
    return RSExportType::Create(Context, T);
  }

  // Following the typedef chain to see whether it's an element name like
  // rs_pixel_rgb or its alias (via typedef).
  while (T != CT) {
    if (T->getTypeClass() != clang::Type::Typedef) {
      break;
    } else {
      const clang::TypedefType *TT = static_cast<const clang::TypedefType*>(T);
      const clang::TypedefDecl *TD = TT->getDecl();
      EI = GetElementInfo(TD->getName());
      if (EI != NULL)
        break;

      T = TD->getUnderlyingType().getTypePtr();
    }
  }

  if (EI == NULL) {
    return RSExportType::Create(Context, T);
  } else {
    return RSExportElement::Create(Context, T, EI);
  }
}

const RSExportElement::ElementInfo *
RSExportElement::GetElementInfo(const llvm::StringRef& Name) {
  if (!Initialized)
    Init();

  ElementInfoMapTy::const_iterator I = ElementInfoMap.find(Name);
  if (I == ElementInfoMap.end())
    return NULL;
  else
    return I->getValue();
}
