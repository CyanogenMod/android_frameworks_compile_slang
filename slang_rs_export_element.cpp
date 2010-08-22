#include "slang_rs_context.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_export_element.hpp"

#include "clang/AST/Decl.h"                 /* for class clang::*Decl */
#include "clang/AST/Type.h"                 /* for class clang::*Type */

#include "clang/Basic/SourceLocation.h"     /* for class clang::SourceLocation */
#include "clang/Basic/IdentifierTable.h"    /* for class clang::IdentifierInfo */

namespace slang {

bool RSExportElement::Initialized = false;
RSExportElement::ElementInfoMapTy RSExportElement::ElementInfoMap;

void RSExportElement::Init() {
    if(!Initialized) {
        /* Initialize ElementInfoMap */
#define USE_ELEMENT_DATA_TYPE
#define USE_ELEMENT_DATA_KIND
#define DEF_ELEMENT(_name, _dk, _dt, _norm, _vsize) \
        {   \
            ElementInfo* EI = new ElementInfo;  \
            EI->kind = GET_ELEMENT_DATA_KIND(_dk);  \
            EI->type = GET_ELEMENT_DATA_TYPE(_dt);  \
            EI->normalized = _norm;  \
            EI->vsize = _vsize;  \
            \
            llvm::StringRef Name(_name);  \
            ElementInfoMap.insert( ElementInfoMapTy::value_type::Create(Name.begin(),   \
                                                                        Name.end(), \
                                                                        ElementInfoMap.getAllocator(),  \
                                                                        EI));   \
        }
#include "slang_rs_export_element_support.inc"

        Initialized = true;
    }
    return;
}

RSExportType* RSExportElement::Create(RSContext* Context, const Type* T, const ElementInfo* EI) {
    /* Create RSExportType corresponded to the @T first and then verify */

    llvm::StringRef TypeName;
    RSExportType* ET = NULL;

    if(!Initialized)
        Init();

    assert(EI != NULL && "Element info not found");

    if(!RSExportType::NormalizeType(T, TypeName))
        return NULL;

    switch(T->getTypeClass()) {
        case Type::Builtin:
        case Type::Pointer:
        {
            assert(EI->vsize == 1 && "Element not a primitive class (please check your macro)");
            RSExportPrimitiveType* EPT = RSExportPrimitiveType::Create(Context, T, TypeName, EI->kind, EI->normalized);
            /* verify */
            assert(EI->type == EPT->getType() && "Element has unexpected type");
            ET = EPT;
        }
        break;

        case Type::ConstantArray:
          {
            //XXX
            break;
          }

        case Type::ExtVector:
        {
            assert(EI->vsize > 1 && "Element not a vector class (please check your macro)");
            RSExportVectorType* EVT = RSExportVectorType::Create(Context,
                                                                 static_cast<ExtVectorType*>(T->getCanonicalTypeInternal().getTypePtr()),
                                                                 TypeName,
                                                                 EI->kind,
                                                                 EI->normalized);
            /* verify */
            assert(EI->type == EVT->getType() && "Element has unexpected type");
            assert(EI->vsize == EVT->getNumElement() && "Element has unexpected size of vector");
            ET = EVT;
        }
        break;

        case Type::Record:
        {
          /* Must be RS object type */

          if ( TypeName.equals(llvm::StringRef("rs_matrix2x2")) ||
               TypeName.equals(llvm::StringRef("rs_matrix3x3")) ||
               TypeName.equals(llvm::StringRef("rs_matrix4x4")) ) {

            const clang::RecordType* RT = static_cast<const RecordType*> (T);
            const RecordDecl* RD = RT->getDecl();
            RD = RD->getDefinition();
            //RSExportRecordType* ERT = new RSExportRecordType(Context, TypeName, RD->hasAttr<PackedAttr>());
            RecordDecl::field_iterator fit = RD->field_begin();
            FieldDecl* FD = *fit;
            const Type* FT = RSExportType::GetTypeOfDecl(FD);
            RSExportConstantArrayType* ECT = RSExportConstantArrayType::Create(Context, static_cast<const ConstantArrayType*> (FT), TypeName);
            ET = ECT;

            //RSExportType* FieldET = RSExportElement::CreateFromDecl(Context, FD);
            //ERT->mFields.push_back( new Field(FieldET, FD->getName(), ERT, 0) );
            //const ASTRecordLayout &ASTRL = Context->getASTContext()->getASTRecordLayout(RD);
            //ERT->AllocSize = (ASTRL.getSize() > ASTRL.getDataSize()) ? (ASTRL.getSize() >> 3) : (ASTRL.getDataSize() >> 3);
            //ET = ERT;

          } else {
            RSExportPrimitiveType* EPT = RSExportPrimitiveType::Create(Context, T, TypeName, EI->kind, EI->normalized);
            /* verify */
            assert(EI->type == EPT->getType() && "Element has unexpected type");
            ET = EPT;
          }
        }
        break;

        default:
            /* TODO: warning: type is not exportable */
            printf("RSExportElement::Create : type '%s' is not exportable\n", T->getTypeClassName());
        break;
    }

    return ET;
}

RSExportType* RSExportElement::CreateFromDecl(RSContext* Context, const DeclaratorDecl* DD) {
    const Type* T = RSExportType::GetTypeOfDecl(DD);
    const Type* CT = GET_CANONICAL_TYPE(T);
    const ElementInfo* EI = NULL;

    /* For rs element that's NOT like those rs_color4f..., just call Create(Context, T) without finding EI */
    /* Note: Those rs_color4f kind of elements are either typed primitive or vector */
    if ((CT->getTypeClass() != Type::Builtin) && (CT->getTypeClass() != Type::ExtVector) && (CT->getTypeClass() != Type::Record)) {
        return RSExportType::Create(Context, T);
    }

    /* Iterative query the name of type to see whether it's an element name like rs_color4f or its alias (via typedef) */
    while (T != CT) {
        if (T->getTypeClass() != Type::Typedef) {
            break;
        } else {
            const TypedefType* TT = static_cast<const TypedefType*>(T);
            const TypedefDecl* TD = TT->getDecl();
            EI = GetElementInfo(TD->getName());
            if (EI != NULL)
                break;

            T = TD->getUnderlyingType().getTypePtr();
        }
    }

    if(EI == NULL) {
        return RSExportType::Create(Context, T);
    } else {
        return RSExportElement::Create(Context, T, EI);
    }
}

const RSExportElement::ElementInfo* RSExportElement::GetElementInfo(const llvm::StringRef& Name) {
    if(!Initialized)
        Init();

    ElementInfoMapTy::const_iterator I = ElementInfoMap.find(Name);
    if(I == ElementInfoMap.end())
        return NULL;
    else
        return I->getValue();
}

}   /* namespace slang */
