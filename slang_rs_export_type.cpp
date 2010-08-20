#include <vector>

#include "slang_rs_context.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_export_element.hpp"

#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"

#include "llvm/ADT/StringExtras.h"  /* for function llvm::utostr_32() */
#include "llvm/Target/TargetData.h" /* for class llvm::TargetData and class llvm::StructLayout */

#include "clang/AST/RecordLayout.h"

namespace slang {

/****************************** RSExportType ******************************/
bool RSExportType::NormalizeType(const Type*& T, llvm::StringRef& TypeName) {
    llvm::SmallPtrSet<const Type*, 8> SPS = llvm::SmallPtrSet<const Type*, 8>();

    if((T = RSExportType::TypeExportable(T, SPS)) == NULL)
        /* TODO: warning the user: type not exportable */
        return false;

    /* Get type name */
    TypeName = RSExportType::GetTypeName(T);
    if(TypeName.empty())
        /* TODO: warning the user: the type is unnmaed */
        return false;

    return true;
}

const Type* RSExportType::GetTypeOfDecl(const DeclaratorDecl* DD) {
    if(DD && DD->getTypeSourceInfo()) {
        QualType T = DD->getTypeSourceInfo()->getType();
        if(T.isNull())
            return NULL;
        else
            return T.getTypePtr();
    }
}

llvm::StringRef RSExportType::GetTypeName(const Type* T) {
    T = GET_CANONICAL_TYPE(T);
    if(T == NULL)
        return llvm::StringRef();

    switch(T->getTypeClass()) {
        case Type::Builtin:
        {
            const BuiltinType* BT = UNSAFE_CAST_TYPE(BuiltinType, T);

            switch(BT->getKind()) {
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type) \
                case builtin_type:  \
                    /* Compiler is smart enough to optimize following *big if branches* since they all become "constant comparison" after macro expansion */  \
                    if(type == RSExportPrimitiveType::DataTypeFloat32) return "float";  \
                    else if(type == RSExportPrimitiveType::DataTypeUnsigned8) return "uchar";   \
                    else if(type == RSExportPrimitiveType::DataTypeUnsigned16) return "ushort"; \
                    else if(type == RSExportPrimitiveType::DataTypeUnsigned32) return "uint";   \
                    else if(type == RSExportPrimitiveType::DataTypeSigned8) return "char";  \
                    else if(type == RSExportPrimitiveType::DataTypeSigned16) return "short";    \
                    else if(type == RSExportPrimitiveType::DataTypeSigned32) return "int";  \
                    else if(type == RSExportPrimitiveType::DataTypeBool) return "bool";  \
                    else assert(false && "Unknow data type of supported builtin");  \
                break;
#include "slang_rs_export_type_support.inc"

                default: assert(false && "Unknow data type of the builtin"); break;
            }
        }
        break;

        case Type::Record:
        {
            const RecordDecl* RD = T->getAsStructureType()->getDecl();
            llvm::StringRef Name = RD->getName();
            if(Name.empty()) {
                if(RD->getTypedefForAnonDecl() != NULL)
                    Name = RD->getTypedefForAnonDecl()->getName();

                if(Name.empty())
                    /* Try to find a name from redeclaration (i.e. typedef) */
                    for(TagDecl::redecl_iterator RI = RD->redecls_begin();
                        RI != RD->redecls_end();
                        RI++)
                    {
                        assert(*RI != NULL && "cannot be NULL object");

                        Name = (*RI)->getName();
                        if(!Name.empty())
                            break;
                    }
            }
            return Name;
        }
        break;

        case Type::Pointer:
        {
            /* "*" plus pointee name */
            const Type* PT = GET_POINTEE_TYPE(T);
            llvm::StringRef PointeeName;
            if(NormalizeType(PT, PointeeName)) {
                char* Name = new char[ 1 /* * */ + PointeeName.size() ];
                Name[0] = '*';
                memcpy(Name + 1, PointeeName.data(), PointeeName.size());
                return Name;
            }
        }
        break;

        case Type::ExtVector:
        {
            const ExtVectorType* EVT = UNSAFE_CAST_TYPE(ExtVectorType, T);
            return RSExportVectorType::GetTypeName(EVT);
        }
        break;

        default:
        break;
    }

    return llvm::StringRef();
}

const Type* RSExportType::TypeExportable(const Type* T, llvm::SmallPtrSet<const Type*, 8>& SPS) {
    /* Normailize first */
    if((T = GET_CANONICAL_TYPE(T)) == NULL)
        return NULL;

    if(SPS.count(T))
        return T;

    switch(T->getTypeClass()) {
        case Type::Builtin:
        {
            const BuiltinType* BT = UNSAFE_CAST_TYPE(BuiltinType, T);

            switch(BT->getKind()) {
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type) \
                case builtin_type:
#include "slang_rs_export_type_support.inc"
                    return T;
                break;

                default:
                    return NULL;
                break;
            }
        }
        break;

        case Type::Record:
        {
            if(RSExportPrimitiveType::GetRSObjectType(T) != RSExportPrimitiveType::DataTypeUnknown)
                return T; /* RS object type, no further checks are needed */

            /* Check internal struct */
            const RecordDecl* RD = T->getAsStructureType()->getDecl();
            if(RD != NULL)
                RD = RD->getDefinition();

            /* Fast check */
            if(RD->hasFlexibleArrayMember() || RD->hasObjectMember())
                return NULL;

            /* Insert myself into checking set */
            SPS.insert(T);

            /* Check all element */
            for(RecordDecl::field_iterator F = RD->field_begin();
                F != RD->field_end();
                F++)
            {
                const Type* FT = GetTypeOfDecl(*F);
                FT = GET_CANONICAL_TYPE(FT);

                if(!TypeExportable(FT, SPS))
                    /*  TODO: warning: unsupported field type */
                    return NULL;
            }

            return T;
        }
        break;

        case Type::Pointer:
        {
            const PointerType* PT = UNSAFE_CAST_TYPE(PointerType, T);
            const Type* PointeeType = GET_POINTEE_TYPE(PT);

            if((PointeeType->getTypeClass() != Type::Pointer) && (TypeExportable(PointeeType, SPS) == NULL) )
                return NULL;
            return T;
        }
        break;

        case Type::ExtVector:
        {
            const ExtVectorType* EVT = UNSAFE_CAST_TYPE(ExtVectorType, T);
            /* Check element numbers */
            if(EVT->getNumElements() < 2 || EVT->getNumElements() > 4)  /* only support vector with size 2, 3 and 4 */
                return NULL;

            /* Check base element type */
            const Type* ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);

            if((ElementType->getTypeClass() != Type::Builtin) || (TypeExportable(ElementType, SPS) == NULL))
                return NULL;
            return T;
        }
        break;

        default:
            return NULL;
        break;
    }
}

RSExportType* RSExportType::Create(RSContext* Context, const Type* T, const llvm::StringRef& TypeName) {
    /*
     * Lookup the context to see whether the type was processed before.
     *  Newly create RSExportType will insert into context in RSExportType::RSExportType()
     */
    RSContext::export_type_iterator ETI = Context->findExportType(TypeName);

    if(ETI != Context->export_types_end())
        return ETI->second;

    RSExportType* ET = NULL;
    switch(T->getTypeClass()) {
        case Type::Record:
            if(RSExportPrimitiveType::GetRSObjectType(TypeName) != RSExportPrimitiveType::DataTypeUnknown)
                ET = RSExportPrimitiveType::Create(Context, T, TypeName);
            else
                ET = RSExportRecordType::Create(Context, T->getAsStructureType(), TypeName);
        break;

        case Type::Builtin:
            ET = RSExportPrimitiveType::Create(Context, T, TypeName);
        break;

        case Type::Pointer:
            ET = RSExportPointerType::Create(Context, UNSAFE_CAST_TYPE(PointerType, T), TypeName);
            /* free the name (allocated in RSExportType::GetTypeName) */
            delete [] TypeName.data();
        break;

        case Type::ExtVector:
            ET = RSExportVectorType::Create(Context, UNSAFE_CAST_TYPE(ExtVectorType, T), TypeName);
        break;

        default:
            /* TODO: warning: type is not exportable */
            printf("RSExportType::Create : type '%s' is not exportable\n", T->getTypeClassName());
        break;
    }

    return ET;
}

RSExportType* RSExportType::Create(RSContext* Context, const Type* T) {
    llvm::StringRef TypeName;
    if(NormalizeType(T, TypeName))
        return Create(Context, T, TypeName);
    else
        return NULL;
}

RSExportType* RSExportType::CreateFromDecl(RSContext* Context, const VarDecl* VD) {
    return RSExportType::Create(Context, GetTypeOfDecl(VD));
}

size_t RSExportType::GetTypeStoreSize(const RSExportType* ET) {
    return ET->getRSContext()->getTargetData()->getTypeStoreSize(ET->getLLVMType());
}

size_t RSExportType::GetTypeAllocSize(const RSExportType* ET) {
    if(ET->getClass() == RSExportType::ExportClassRecord)
        return static_cast<const RSExportRecordType*>(ET)->getAllocSize();
    else
        return ET->getRSContext()->getTargetData()->getTypeAllocSize(ET->getLLVMType());
}

RSExportType::RSExportType(RSContext* Context, const llvm::StringRef& Name) :
    mContext(Context),
    mName(Name.data(), Name.size()), /* make a copy on Name since data of @Name which is stored in ASTContext will be destroyed later */
    mLLVMType(NULL)
{
    /* TODO: need to check whether the insertion is successful or not */
    Context->insertExportType(Name, this);
    return;
}

/****************************** RSExportPrimitiveType ******************************/
RSExportPrimitiveType::RSObjectTypeMapTy* RSExportPrimitiveType::RSObjectTypeMap = NULL;
llvm::Type* RSExportPrimitiveType::RSObjectLLVMType = NULL;

bool RSExportPrimitiveType::IsPrimitiveType(const Type* T) {
    if((T != NULL) && (T->getTypeClass() == Type::Builtin))
        return true;
    else
        return false;
}

RSExportPrimitiveType::DataType RSExportPrimitiveType::GetRSObjectType(const llvm::StringRef& TypeName) {
    if(TypeName.empty())
        return DataTypeUnknown;

    if(RSObjectTypeMap == NULL) {
        RSObjectTypeMap = new RSObjectTypeMapTy(16);

#define USE_ELEMENT_DATA_TYPE
#define DEF_RS_OBJECT_TYPE(type, name)  \
        RSObjectTypeMap->GetOrCreateValue(name, GET_ELEMENT_DATA_TYPE(type));
#include "slang_rs_export_element_support.inc"
    }

    RSObjectTypeMapTy::const_iterator I = RSObjectTypeMap->find(TypeName);
    if(I == RSObjectTypeMap->end())
        return DataTypeUnknown;
    else
        return I->getValue();
}

RSExportPrimitiveType::DataType RSExportPrimitiveType::GetRSObjectType(const Type* T) {
    T = GET_CANONICAL_TYPE(T);
    if((T == NULL) || (T->getTypeClass() != Type::Record))
        return DataTypeUnknown;

    return GetRSObjectType( RSExportType::GetTypeName(T) );
}

const size_t RSExportPrimitiveType::SizeOfDataTypeInBits[RSExportPrimitiveType::DataTypeMax + 1] = {
    0,
    16, /* DataTypeFloat16 */
    32, /* DataTypeFloat32 */
    64, /* DataTypeFloat64 */
    8, /* DataTypeSigned8 */
    16, /* DataTypeSigned16 */
    32, /* DataTypeSigned32 */
    64, /* DataTypeSigned64 */
    8, /* DataTypeUnsigned8 */
    16, /* DataTypeUnsigned16 */
    32, /* DataTypeUnsigned32 */
    64, /* DataTypeUnSigned64 */

    16, /* DataTypeUnsigned565 */
    16, /* DataTypeUnsigned5551 */
    16, /* DataTypeUnsigned4444 */

    1, /* DataTypeBool */

    32, /* DataTypeRSElement */
    32, /* DataTypeRSType */
    32, /* DataTypeRSAllocation */
    32, /* DataTypeRSSampler */
    32, /* DataTypeRSScript */
    32, /* DataTypeRSMesh */
    32, /* DataTypeRSProgramFragment */
    32, /* DataTypeRSProgramVertex */
    32, /* DataTypeRSProgramRaster */
    32, /* DataTypeRSProgramStore */
    32, /* DataTypeRSFont */
    128, /* DataTypeRSMatrix2x2 */
    288, /* DataTypeRSMatrix3x3 */
    512, /* DataTypeRSMatrix4x4 */
    0
};

size_t RSExportPrimitiveType::GetSizeInBits(const RSExportPrimitiveType* EPT) {
    assert(((EPT->getType() >= DataTypeFloat32) && (EPT->getType() < DataTypeMax)) && "RSExportPrimitiveType::GetSizeInBits : unknown data type");
    return SizeOfDataTypeInBits[ static_cast<int>(EPT->getType()) ];
}

RSExportPrimitiveType::DataType RSExportPrimitiveType::GetDataType(const Type* T) {
    if(T == NULL)
        return DataTypeUnknown;

    switch(T->getTypeClass()) {
        case Type::Builtin:
        {
            const BuiltinType* BT = UNSAFE_CAST_TYPE(BuiltinType, T);
            switch(BT->getKind()) {
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type) \
                case builtin_type:  \
                    return type;  \
                break;
#include "slang_rs_export_type_support.inc"

                /* The size of types Long, ULong and WChar depend on platform so we abandon the support to them */
                /* Type of its size exceeds 32 bits (e.g. int64_t, double, etc.) does not support */

                default:
                    /* TODO: warning the user: unsupported type */
                    printf("RSExportPrimitiveType::GetDataType : built-in type has no corresponding data type for built-in type");
                break;
            }
        }
        break;

        case Type::Record:
            /* must be RS object type */
            return RSExportPrimitiveType::GetRSObjectType(T);
        break;

        default:
            printf("RSExportPrimitiveType::GetDataType : type '%s' is not supported primitive type", T->getTypeClassName());
        break;
    }

    return DataTypeUnknown;
}

RSExportPrimitiveType* RSExportPrimitiveType::Create(RSContext* Context, const Type* T, const llvm::StringRef& TypeName, DataKind DK, bool Normalized) {
    DataType DT = GetDataType(T);

    if((DT == DataTypeUnknown) || TypeName.empty())
        return NULL;
    else
        return new RSExportPrimitiveType(Context, TypeName, DT, DK, Normalized);
}

RSExportPrimitiveType* RSExportPrimitiveType::Create(RSContext* Context, const Type* T, DataKind DK) {
    llvm::StringRef TypeName;
    if(RSExportType::NormalizeType(T, TypeName) && IsPrimitiveType(T))
        return Create(Context, T, TypeName, DK);
    else
        return NULL;
}

RSExportType::ExportClass RSExportPrimitiveType::getClass() const {
    return RSExportType::ExportClassPrimitive;
}

const llvm::Type* RSExportPrimitiveType::convertToLLVMType() const {
    llvm::LLVMContext& C = getRSContext()->getLLVMContext();

    if(isRSObjectType()) {
        /* struct { int* p; } __attribute__((packed, aligned(pointer_size))) which is <{ [1 x i32] }> in LLVM */
        if(RSObjectLLVMType == NULL) {
            std::vector<const llvm::Type*> Elements;
            Elements.push_back( llvm::ArrayType::get(llvm::Type::getInt32Ty(C), 1) );
            RSObjectLLVMType = llvm::StructType::get(C, Elements, true);
        }
        return RSObjectLLVMType;
    }

    switch(mType) {
        case DataTypeFloat32:
            return llvm::Type::getFloatTy(C);
        break;

        case DataTypeSigned8:
        case DataTypeUnsigned8:
            return llvm::Type::getInt8Ty(C);
        break;

        case DataTypeSigned16:
        case DataTypeUnsigned16:
        case DataTypeUnsigned565:
        case DataTypeUnsigned5551:
        case DataTypeUnsigned4444:
            return llvm::Type::getInt16Ty(C);
        break;

        case DataTypeSigned32:
        case DataTypeUnsigned32:
            return llvm::Type::getInt32Ty(C);
        break;

        case DataTypeBool:
            return llvm::Type::getInt1Ty(C);
        break;

        default:
            assert(false && "Unknown data type");
        break;
    }

    return NULL;
}

/****************************** RSExportPointerType ******************************/

const Type* RSExportPointerType::IntegerType = NULL;

RSExportPointerType* RSExportPointerType::Create(RSContext* Context, const PointerType* PT, const llvm::StringRef& TypeName) {
    const Type* PointeeType = GET_POINTEE_TYPE(PT);
    const RSExportType* PointeeET;

    if(PointeeType->getTypeClass() != Type::Pointer) {
        PointeeET = RSExportType::Create(Context, PointeeType);
    } else {
        /* Double or higher dimension of pointer, export as int* */
        assert(IntegerType != NULL && "Built-in integer type is not set");
        PointeeET = RSExportPrimitiveType::Create(Context, IntegerType);
    }

    if(PointeeET == NULL) {
        printf("Failed to create type for pointee");
        return NULL;
    }

    return new RSExportPointerType(Context, TypeName, PointeeET);
}

RSExportType::ExportClass RSExportPointerType::getClass() const {
    return RSExportType::ExportClassPointer;
}

const llvm::Type* RSExportPointerType::convertToLLVMType() const {
    const llvm::Type* PointeeType = mPointeeType->getLLVMType();
    return llvm::PointerType::getUnqual(PointeeType);
}

/****************************** RSExportVectorType ******************************/
const char* RSExportVectorType::VectorTypeNameStore[][3] = {
    /* 0 */ { "char2",      "char3",    "char4" },
    /* 1 */ { "uchar2",     "uchar3",   "uchar4" },
    /* 2 */ { "short2",     "short3",   "short4" },
    /* 3 */ { "ushort2",    "ushort3",  "ushort4" },
    /* 4 */ { "int2",       "int3",     "int4" },
    /* 5 */ { "uint2",      "uint3",    "uint4" },
    /* 6 */ { "float2",     "float3",   "float4" },
};

llvm::StringRef RSExportVectorType::GetTypeName(const ExtVectorType* EVT) {
    const Type* ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);

    if((ElementType->getTypeClass() != Type::Builtin))
        return llvm::StringRef();

    const BuiltinType* BT = UNSAFE_CAST_TYPE(BuiltinType, ElementType);
    const char** BaseElement = NULL;

    switch(BT->getKind()) {
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type) \
        case builtin_type:  \
            /* Compiler is smart enough to optimize following *big if branches* since they all become "constant comparison" after macro expansion */  \
            if(type == RSExportPrimitiveType::DataTypeSigned8) BaseElement = VectorTypeNameStore[0];    \
            else if(type == RSExportPrimitiveType::DataTypeUnsigned8) BaseElement = VectorTypeNameStore[1]; \
            else if(type == RSExportPrimitiveType::DataTypeSigned16) BaseElement = VectorTypeNameStore[2];  \
            else if(type == RSExportPrimitiveType::DataTypeUnsigned16) BaseElement = VectorTypeNameStore[3];    \
            else if(type == RSExportPrimitiveType::DataTypeSigned32) BaseElement = VectorTypeNameStore[4];  \
            else if(type == RSExportPrimitiveType::DataTypeUnsigned32) BaseElement = VectorTypeNameStore[5];    \
            else if(type == RSExportPrimitiveType::DataTypeFloat32) BaseElement = VectorTypeNameStore[6];   \
            else if(type == RSExportPrimitiveType::DataTypeBool) BaseElement = VectorTypeNameStore[0];   \
        break;
#include "slang_rs_export_type_support.inc"

        default: return llvm::StringRef(); break;
    }

    if((BaseElement != NULL) && (EVT->getNumElements() > 1) && (EVT->getNumElements() <= 4))
        return BaseElement[EVT->getNumElements() - 2];
    else
        return llvm::StringRef();
}

RSExportVectorType* RSExportVectorType::Create(RSContext* Context, const ExtVectorType* EVT, const llvm::StringRef& TypeName, DataKind DK, bool Normalized) {
    assert(EVT != NULL && EVT->getTypeClass() == Type::ExtVector);

    const Type* ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);
    RSExportPrimitiveType::DataType DT = RSExportPrimitiveType::GetDataType(ElementType);

    if(DT != RSExportPrimitiveType::DataTypeUnknown)
        return new RSExportVectorType(Context, TypeName, DT, DK, Normalized, EVT->getNumElements());
    else
        printf("RSExportVectorType::Create : unsupported base element type\n");
}

RSExportType::ExportClass RSExportVectorType::getClass() const {
    return RSExportType::ExportClassVector;
}

const llvm::Type* RSExportVectorType::convertToLLVMType() const {
    const llvm::Type* ElementType = RSExportPrimitiveType::convertToLLVMType();
    return llvm::VectorType::get(ElementType, getNumElement());
}

/****************************** RSExportRecordType ******************************/
RSExportRecordType* RSExportRecordType::Create(RSContext* Context, const RecordType* RT, const llvm::StringRef& TypeName, bool mIsArtificial) {
    assert(RT != NULL && RT->getTypeClass() == Type::Record);

    const RecordDecl* RD = RT->getDecl();
    assert(RD->isStruct());

    RD = RD->getDefinition();
    if(RD == NULL) {
        /* TODO: warning: actual struct definition isn't declared in this moudle */
        printf("RSExportRecordType::Create : this struct is not defined in this module.");
        return NULL;
    }

    RSExportRecordType* ERT = new RSExportRecordType(Context, TypeName, RD->hasAttr<PackedAttr>(), mIsArtificial);
    unsigned int Index = 0;

    for(RecordDecl::field_iterator fit = RD->field_begin();
        fit != RD->field_end();
        fit++, Index++)
    {
#define FAILED_CREATE_FIELD(err)    do {    \
                                        if(*err) \
                                            printf("RSExportRecordType::Create : failed to create field (%s)\n", err);   \
                                        delete ERT;  \
                                        return NULL;    \
                                    } while(false)
        /* FIXME: All fields should be primitive type */
        assert((*fit)->getKind() == Decl::Field);
        FieldDecl* FD = *fit;

        /* We don't support bit field TODO: allow bitfield with size 8, 16, 32 */
        if(FD->isBitField())
            FAILED_CREATE_FIELD("bit field is not supported");

        /* Type */
        RSExportType* ET = RSExportElement::CreateFromDecl(Context, FD);

        if(ET != NULL)
            ERT->mFields.push_back( new Field(ET, FD->getName(), ERT, Index) );
        else
            FAILED_CREATE_FIELD(FD->getName().str().c_str());
#undef FAILED_CREATE_FIELD
    }


    const ASTRecordLayout &ASTRL = Context->getASTContext()->getASTRecordLayout(RD);
    ERT->AllocSize = (ASTRL.getSize() > ASTRL.getDataSize()) ? (ASTRL.getSize() >> 3) : (ASTRL.getDataSize() >> 3);

    return ERT;
}

RSExportType::ExportClass RSExportRecordType::getClass() const {
    return RSExportType::ExportClassRecord;
}

const llvm::Type* RSExportRecordType::convertToLLVMType() const {
    std::vector<const llvm::Type*> FieldTypes;

    for(const_field_iterator FI = fields_begin();
        FI != fields_end();
        FI++)
    {
        const Field* F = *FI;
        const RSExportType* FET = F->getType();

        FieldTypes.push_back(FET->getLLVMType());
    }

    return llvm::StructType::get(getRSContext()->getLLVMContext(), FieldTypes, mIsPacked);
}

/****************************** RSExportRecordType::Field ******************************/
size_t RSExportRecordType::Field::getOffsetInParent() const {
    /* Struct layout obtains below will be cached by LLVM */
    const llvm::StructLayout* SL = mParent->getRSContext()->getTargetData()->getStructLayout(static_cast<const llvm::StructType*>(mParent->getLLVMType()));
    return SL->getElementOffset(mIndex);
}

}   /* namespace slang */
