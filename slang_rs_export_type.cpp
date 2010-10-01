#include "slang_rs_context.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_export_element.hpp"

#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Target/TargetData.h"

#include "clang/AST/RecordLayout.h"

#include <vector>

using namespace slang;

/****************************** RSExportType ******************************/
bool RSExportType::NormalizeType(const clang::Type *&T,
                                 llvm::StringRef &TypeName) {
  llvm::SmallPtrSet<const clang::Type*, 8> SPS =
      llvm::SmallPtrSet<const clang::Type*, 8>();

  if ((T = RSExportType::TypeExportable(T, SPS)) == NULL)
    // TODO: warning the user: type not exportable
    return false;

  // Get type name
  TypeName = RSExportType::GetTypeName(T);
  if (TypeName.empty())
    // TODO: warning the user: the type is unnamed
    return false;

  return true;
}

const clang::Type
*RSExportType::GetTypeOfDecl(const clang::DeclaratorDecl *DD) {
  if (DD && DD->getTypeSourceInfo()) {
    clang::QualType T = DD->getTypeSourceInfo()->getType();
    if (T.isNull())
      return NULL;
    else
      return T.getTypePtr();
  }
  return NULL;
}

llvm::StringRef RSExportType::GetTypeName(const clang::Type* T) {
  T = GET_CANONICAL_TYPE(T);
  if (T == NULL)
    return llvm::StringRef();

  switch (T->getTypeClass()) {
    case clang::Type::Builtin: {
      const clang::BuiltinType *BT = UNSAFE_CAST_TYPE(clang::BuiltinType, T);

      switch (BT->getKind()) {
        // Compiler is smart enough to optimize following *big if branches*
        // since they all become "constant comparison" after macro expansion
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type)       \
        case builtin_type: {                                    \
          if (type == RSExportPrimitiveType::DataTypeFloat32)           \
            return "float";                                             \
          else if (type == RSExportPrimitiveType::DataTypeFloat64)      \
            return "double";                                            \
          else if (type == RSExportPrimitiveType::DataTypeUnsigned8)    \
            return "uchar";                                             \
          else if (type == RSExportPrimitiveType::DataTypeUnsigned16)   \
            return "ushort";                                            \
          else if (type == RSExportPrimitiveType::DataTypeUnsigned32)   \
            return "uint";                                              \
          else if (type == RSExportPrimitiveType::DataTypeSigned8)      \
            return "char";                                              \
          else if (type == RSExportPrimitiveType::DataTypeSigned16)     \
            return "short";                                             \
          else if (type == RSExportPrimitiveType::DataTypeSigned32)     \
            return "int";                                               \
          else if(type == RSExportPrimitiveType::DataTypeSigned64)      \
            return "long";                                              \
          else if (type == RSExportPrimitiveType::DataTypeBool)         \
            return "bool";                                              \
          else                                                          \
            assert(false && "Unknow data type of supported builtin");   \
          break;                                                        \
        }
#include "slang_rs_export_type_support.inc"

          default: {
            assert(false && "Unknown data type of the builtin");
            break;
          }
        }
      break;
    }

    case clang::Type::Record: {
      const clang::RecordDecl *RD = T->getAsStructureType()->getDecl();
      llvm::StringRef Name = RD->getName();
      if (Name.empty()) {
          if (RD->getTypedefForAnonDecl() != NULL)
            Name = RD->getTypedefForAnonDecl()->getName();

          if(Name.empty())
            // Try to find a name from redeclaration (i.e. typedef)
            for (clang::TagDecl::redecl_iterator RI = RD->redecls_begin(),
                     RE = RD->redecls_end();
                 RI != RE;
                 RI++) {
              assert(*RI != NULL && "cannot be NULL object");

              Name = (*RI)->getName();
              if (!Name.empty())
                break;
            }
      }
      return Name;
    }

    case clang::Type::Pointer: {
      // "*" plus pointee name
      const clang::Type* PT = GET_POINTEE_TYPE(T);
      llvm::StringRef PointeeName;
      if (NormalizeType(PT, PointeeName)) {
        char *Name = new char[ 1 /* * */ + PointeeName.size() + 1 ];
        Name[0] = '*';
        memcpy(Name + 1, PointeeName.data(), PointeeName.size());
        Name[PointeeName.size() + 1] = '\0';
        return Name;
      }
      break;
    }

    case clang::Type::ConstantArray: {
      const clang::ConstantArrayType *ECT =
          UNSAFE_CAST_TYPE(clang::ConstantArrayType, T);
      return RSExportConstantArrayType::GetTypeName(ECT);
      break;
    }

    case clang::Type::ExtVector: {
      const clang::ExtVectorType *EVT =
          UNSAFE_CAST_TYPE(clang::ExtVectorType, T);
      return RSExportVectorType::GetTypeName(EVT);
      break;
    }

    default: {
      break;
    }
  }

  return llvm::StringRef();
}

const clang::Type *RSExportType::TypeExportable(
    const clang::Type *T,
    llvm::SmallPtrSet<const clang::Type*, 8>& SPS) {
  // Normalize first
  if ((T = GET_CANONICAL_TYPE(T)) == NULL)
    return NULL;

  if (SPS.count(T))
    return T;

  switch (T->getTypeClass()) {
    case clang::Type::Builtin: {
      const clang::BuiltinType *BT = UNSAFE_CAST_TYPE(clang::BuiltinType, T);

      switch (BT->getKind()) {
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type)       \
        case builtin_type:
#include "slang_rs_export_type_support.inc"
        {
          return T;
        }

        default: {
          return NULL;
        }
      }
      // Never be here
    }


    case clang::Type::Record: {
      if (RSExportPrimitiveType::GetRSObjectType(T) !=
          RSExportPrimitiveType::DataTypeUnknown)
        return T; // RS object type, no further checks are needed

      // Check internal struct
      const clang::RecordDecl *RD = T->getAsStructureType()->getDecl();
      if (RD != NULL)
        RD = RD->getDefinition();

      // Fast check
      if (RD->hasFlexibleArrayMember() || RD->hasObjectMember())
        return NULL;

      // Insert myself into checking set
      SPS.insert(T);

      // Check all element
      for (clang::RecordDecl::field_iterator FI = RD->field_begin(),
               FE = RD->field_end();
           FI != FE;
           FI++) {
        const clang::Type* FT = GetTypeOfDecl(*FI);
        FT = GET_CANONICAL_TYPE(FT);

        if (!TypeExportable(FT, SPS))
          // TODO: warning: unsupported field type
          return NULL;
      }

      return T;
    }

    case clang::Type::Pointer: {
      const clang::PointerType *PT = UNSAFE_CAST_TYPE(clang::PointerType, T);
      const clang::Type *PointeeType = GET_POINTEE_TYPE(PT);

      if ((PointeeType->getTypeClass() != clang::Type::Pointer) &&
         (TypeExportable(PointeeType, SPS) == NULL) )
        return NULL;
      else
        return T;
    }

    case clang::Type::ConstantArray: {
      const clang::ConstantArrayType *ECT =
          UNSAFE_CAST_TYPE(clang::ConstantArrayType, T);
      // No longer only support 2x2, 3x3 and 4x4 arrays
      // if (ECT->getNumElements() != 4 &&
      //     ECT->getNumElements() != 9 &&
      //     ECT->getNumElements() != 16)
      //  return NULL;

      // Check base element type
      const clang::Type *ElementType = GET_CONSTANT_ARRAY_ELEMENT_TYPE(ECT);

      if ((ElementType->getTypeClass() != clang::Type::Builtin) ||
          (TypeExportable(ElementType, SPS) == NULL))
        return NULL;
      else
        return T;
    }

    case clang::Type::ExtVector: {
      const clang::ExtVectorType *EVT =
          UNSAFE_CAST_TYPE(clang::ExtVectorType, T);
      // Only vector with size 2, 3 and 4 are supported.
      if (EVT->getNumElements() < 2 || EVT->getNumElements() > 4)
        return NULL;

      // Check base element type
      const clang::Type *ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);

      if ((ElementType->getTypeClass() != clang::Type::Builtin) ||
          (TypeExportable(ElementType, SPS) == NULL))
        return NULL;
      else
        return T;
    }

    default: {
      return NULL;
    }
  }
}

RSExportType *RSExportType::Create(RSContext *Context,
                                   const clang::Type *T,
                                   const llvm::StringRef &TypeName) {
  // Lookup the context to see whether the type was processed before.
  // Newly created RSExportType will insert into context
  // in RSExportType::RSExportType()
  RSContext::export_type_iterator ETI = Context->findExportType(TypeName);

  if (ETI != Context->export_types_end())
    return ETI->second;

  RSExportType *ET = NULL;
  switch (T->getTypeClass()) {
    case clang::Type::Record: {
      RSExportPrimitiveType::DataType dt =
          RSExportPrimitiveType::GetRSObjectType(TypeName);
      switch (dt) {
        case RSExportPrimitiveType::DataTypeUnknown: {  // User-defined types
          ET = RSExportRecordType::Create(Context,
                                          T->getAsStructureType(),
                                          TypeName);
          break;
        }
        case RSExportPrimitiveType::DataTypeRSMatrix2x2:
        case RSExportPrimitiveType::DataTypeRSMatrix3x3:
        case RSExportPrimitiveType::DataTypeRSMatrix4x4: {
          const clang::RecordType *RT =
              static_cast<const clang::RecordType*> (T);
          const clang::RecordDecl *RD = RT->getDecl();
          RD = RD->getDefinition();
          clang::RecordDecl::field_iterator FI = RD->field_begin();
          clang::FieldDecl *FD = *FI;
          const clang::Type *FT = RSExportType::GetTypeOfDecl(FD);
          ET = RSExportConstantArrayType::Create(
              Context,
              static_cast<const clang::ConstantArrayType*> (FT),
              TypeName);
          break;
        }
        default: {
          ET = RSExportPrimitiveType::Create(Context, T, TypeName);
          break;
        }
      }
      break;
    }
    case clang::Type::Builtin: {
      ET = RSExportPrimitiveType::Create(Context, T, TypeName);
      break;
    }
    case clang::Type::Pointer: {
      ET = RSExportPointerType::Create(Context,
                                       UNSAFE_CAST_TYPE(clang::PointerType, T),
                                       TypeName);
      // Free the name (allocated in RSExportType::GetTypeName)
      delete [] TypeName.data();
      break;
    }
    case clang::Type::ConstantArray: {
      ET = RSExportConstantArrayType::Create(
          Context,
          UNSAFE_CAST_TYPE(clang::ConstantArrayType, T),
          TypeName);
      break;
    }
    case clang::Type::ExtVector: {
      ET = RSExportVectorType::Create(Context,
                                      UNSAFE_CAST_TYPE(clang::ExtVectorType, T),
                                      TypeName);
      break;
    }
    default: {
      // TODO: warning: type is not exportable
      fprintf(stderr,
              "RSExportType::Create : type '%s' is not exportable\n",
              T->getTypeClassName());
      break;
    }
  }

  return ET;
}

RSExportType* RSExportType::Create(RSContext *Context, const clang::Type *T) {
  llvm::StringRef TypeName;
  if (NormalizeType(T, TypeName))
    return Create(Context, T, TypeName);
  else
    return NULL;
}

RSExportType *RSExportType::CreateFromDecl(RSContext *Context,
                                           const clang::VarDecl *VD) {
  return RSExportType::Create(Context, GetTypeOfDecl(VD));
}

size_t RSExportType::GetTypeStoreSize(const RSExportType *ET) {
  return ET->getRSContext()->getTargetData()->getTypeStoreSize(
      ET->getLLVMType()
                                                               );
}

size_t RSExportType::GetTypeAllocSize(const RSExportType *ET) {
  if (ET->getClass() == RSExportType::ExportClassRecord)
    return static_cast<const RSExportRecordType*>(ET)->getAllocSize();
  else
    return ET->getRSContext()->getTargetData()->getTypeAllocSize(
        ET->getLLVMType()
                                                                 );
}

RSExportType::RSExportType(RSContext *Context, const llvm::StringRef &Name) :
    mContext(Context),
    // Make a copy on Name since data of @Name which is stored in ASTContext
    // will be destroyed later
    mName(Name.data(), Name.size()),
    mLLVMType(NULL) {
  // TODO: need to check whether the insertion is successful or not
  Context->insertExportType(Name, this);
  return;
}

/************************** RSExportPrimitiveType **************************/
RSExportPrimitiveType::RSObjectTypeMapTy
*RSExportPrimitiveType::RSObjectTypeMap = NULL;

llvm::Type* RSExportPrimitiveType::RSObjectLLVMType = NULL;

bool RSExportPrimitiveType::IsPrimitiveType(const clang::Type *T) {
  if ((T != NULL) && (T->getTypeClass() == clang::Type::Builtin))
    return true;
  else
    return false;
}

RSExportPrimitiveType::DataType
RSExportPrimitiveType::GetRSObjectType(const llvm::StringRef &TypeName) {
  if (TypeName.empty())
    return DataTypeUnknown;

  if (RSObjectTypeMap == NULL) {
    RSObjectTypeMap = new RSObjectTypeMapTy(16);

#define USE_ELEMENT_DATA_TYPE
#define DEF_RS_OBJECT_TYPE(type, name)                                  \
    RSObjectTypeMap->GetOrCreateValue(name, GET_ELEMENT_DATA_TYPE(type));
#include "slang_rs_export_element_support.inc"
  }

  RSObjectTypeMapTy::const_iterator I = RSObjectTypeMap->find(TypeName);
  if (I == RSObjectTypeMap->end())
    return DataTypeUnknown;
  else
    return I->getValue();
}

RSExportPrimitiveType::DataType
RSExportPrimitiveType::GetRSObjectType(const clang::Type *T) {
  T = GET_CANONICAL_TYPE(T);
  if ((T == NULL) || (T->getTypeClass() != clang::Type::Record))
    return DataTypeUnknown;

  return GetRSObjectType( RSExportType::GetTypeName(T) );
}

const size_t
RSExportPrimitiveType::SizeOfDataTypeInBits[
    RSExportPrimitiveType::DataTypeMax + 1] = {
  0,
  16, // DataTypeFloat16
  32, // DataTypeFloat32
  64, // DataTypeFloat64
  8,  // DataTypeSigned8
  16, // DataTypeSigned16
  32, // DataTypeSigned32
  64, // DataTypeSigned64
  8,  // DataTypeUnsigned8
  16, // DataTypeUnsigned16
  32, // DataTypeUnsigned32
  64, // DataTypeUnSigned64

  16, // DataTypeUnsigned565
  16, // DataTypeUnsigned5551
  16, // DataTypeUnsigned4444

  1,  // DataTypeBool

  32, // DataTypeRSElement
  32, // DataTypeRSType
  32, // DataTypeRSAllocation
  32, // DataTypeRSSampler
  32, // DataTypeRSScript
  32, // DataTypeRSMesh
  32, // DataTypeRSProgramFragment
  32, // DataTypeRSProgramVertex
  32, // DataTypeRSProgramRaster
  32, // DataTypeRSProgramStore
  32, // DataTypeRSFont
  128, // DataTypeRSMatrix2x2
  288, // DataTypeRSMatrix3x3
  512, // DataTypeRSMatrix4x4
  0
};

size_t RSExportPrimitiveType::GetSizeInBits(const RSExportPrimitiveType *EPT) {
  assert(((EPT->getType() >= DataTypeFloat32) &&
          (EPT->getType() < DataTypeMax)) &&
         "RSExportPrimitiveType::GetSizeInBits : unknown data type");
  return SizeOfDataTypeInBits[ static_cast<int>(EPT->getType()) ];
}

RSExportPrimitiveType::DataType
RSExportPrimitiveType::GetDataType(const clang::Type* T) {
  if (T == NULL)
    return DataTypeUnknown;

  switch (T->getTypeClass()) {
    case clang::Type::Builtin: {
      const clang::BuiltinType *BT = UNSAFE_CAST_TYPE(clang::BuiltinType, T);
      switch (BT->getKind()) {
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type)       \
        case builtin_type: {                                    \
          return type;                                          \
          break;                                                \
        }
#include "slang_rs_export_type_support.inc"

        // The size of types Long, ULong and WChar depend on platform so we
        // abandon the support to them. Type of its size exceeds 32 bits (e.g.
        // int64_t, double, etc.): no support

        default: {
          // TODO: warning the user: unsupported type
          fprintf(stderr, "RSExportPrimitiveType::GetDataType : built-in type "
                          "has no corresponding data type for built-in type");
          break;
        }
      }
      break;
    }

    case clang::Type::Record: {
      // must be RS object type
      return RSExportPrimitiveType::GetRSObjectType(T);
      break;
    }

    default: {
      fprintf(stderr, "RSExportPrimitiveType::GetDataType : type '%s' is not "
                      "supported primitive type", T->getTypeClassName());
      break;
    }
  }

  return DataTypeUnknown;
}

RSExportPrimitiveType
*RSExportPrimitiveType::Create(RSContext *Context,
                               const clang::Type *T,
                               const llvm::StringRef &TypeName,
                               DataKind DK,
                               bool Normalized) {
  DataType DT = GetDataType(T);

  if ((DT == DataTypeUnknown) || TypeName.empty())
    return NULL;
  else
    return new RSExportPrimitiveType(Context, TypeName, DT, DK, Normalized);
}

RSExportPrimitiveType *RSExportPrimitiveType::Create(RSContext *Context,
                                                     const clang::Type *T,
                                                     DataKind DK) {
  llvm::StringRef TypeName;
  if (RSExportType::NormalizeType(T, TypeName) && IsPrimitiveType(T))
    return Create(Context, T, TypeName, DK);
  else
    return NULL;
}

RSExportType::ExportClass RSExportPrimitiveType::getClass() const {
  return RSExportType::ExportClassPrimitive;
}

const llvm::Type *RSExportPrimitiveType::convertToLLVMType() const {
  llvm::LLVMContext &C = getRSContext()->getLLVMContext();

  if (isRSObjectType()) {
    // struct {
    //   int *p;
    // } __attribute__((packed, aligned(pointer_size)))
    //
    // which is
    //
    // <{ [1 x i32] }> in LLVM
    //
    if (RSObjectLLVMType == NULL) {
      std::vector<const llvm::Type *> Elements;
      Elements.push_back( llvm::ArrayType::get(llvm::Type::getInt32Ty(C), 1) );
      RSObjectLLVMType = llvm::StructType::get(C, Elements, true);
    }
    return RSObjectLLVMType;
  }

  switch (mType) {
    case DataTypeFloat32: {
      return llvm::Type::getFloatTy(C);
      break;
    }
    case DataTypeFloat64: {
      return llvm::Type::getDoubleTy(C);
      break;
    }
    case DataTypeSigned8:
    case DataTypeUnsigned8: {
      return llvm::Type::getInt8Ty(C);
      break;
    }
    case DataTypeSigned16:
    case DataTypeUnsigned16:
    case DataTypeUnsigned565:
    case DataTypeUnsigned5551:
    case DataTypeUnsigned4444: {
      return llvm::Type::getInt16Ty(C);
      break;
    }
    case DataTypeSigned32:
    case DataTypeUnsigned32: {
      return llvm::Type::getInt32Ty(C);
      break;
    }
    case DataTypeSigned64: {
    // case DataTypeUnsigned64:
      return llvm::Type::getInt64Ty(C);
      break;
    }
    case DataTypeBool: {
      return llvm::Type::getInt1Ty(C);
      break;
    }
    default: {
      assert(false && "Unknown data type");
    }
  }

  return NULL;
}

/**************************** RSExportPointerType ****************************/

const clang::Type *RSExportPointerType::IntegerType = NULL;

RSExportPointerType
*RSExportPointerType::Create(RSContext *Context,
                             const clang::PointerType *PT,
                             const llvm::StringRef &TypeName) {
  const clang::Type *PointeeType = GET_POINTEE_TYPE(PT);
  const RSExportType *PointeeET;

  if (PointeeType->getTypeClass() != clang::Type::Pointer) {
    PointeeET = RSExportType::Create(Context, PointeeType);
  } else {
    // Double or higher dimension of pointer, export as int*
    assert(IntegerType != NULL && "Built-in integer type is not set");
    PointeeET = RSExportPrimitiveType::Create(Context, IntegerType);
  }

  if (PointeeET == NULL) {
    fprintf(stderr, "Failed to create type for pointee");
    return NULL;
  }

  return new RSExportPointerType(Context, TypeName, PointeeET);
}

RSExportType::ExportClass RSExportPointerType::getClass() const {
  return RSExportType::ExportClassPointer;
}

const llvm::Type *RSExportPointerType::convertToLLVMType() const {
  const llvm::Type *PointeeType = mPointeeType->getLLVMType();
  return llvm::PointerType::getUnqual(PointeeType);
}

/************************* RSExportConstantArrayType *************************/
llvm::StringRef
RSExportConstantArrayType::GetTypeName(const clang::ConstantArrayType *CT) {
  llvm::APInt i = CT->getSize();
  if (i == 4) {
    return llvm::StringRef("rs_matrix2x2");
  } else if (i == 9) {
    return llvm::StringRef("rs_matrix3x3");
  } else if (i == 16) {
    return llvm::StringRef("rs_matrix4x4");
  }
  return llvm::StringRef();
}

RSExportConstantArrayType
*RSExportConstantArrayType::Create(RSContext *Context,
                                   const clang::ConstantArrayType *CT,
                                   const llvm::StringRef &TypeName,
                                   DataKind DK,
                                   bool Normalized) {
  assert(CT != NULL && CT->getTypeClass() == clang::Type::ConstantArray);

  int64_t Size = CT->getSize().getSExtValue();
  RSExportPrimitiveType::DataType DT;
  if (Size == 4) {
    DT = RSExportPrimitiveType::DataTypeRSMatrix2x2;
  } else if (Size == 9) {
    DT = RSExportPrimitiveType::DataTypeRSMatrix3x3;
  } else if (Size == 16) {
    DT = RSExportPrimitiveType::DataTypeRSMatrix4x4;
  } else {
    fprintf(stderr, "RSExportConstantArrayType::Create : unsupported base "
                    "element type\n");
    return NULL;
  }

  return new RSExportConstantArrayType(Context,
                                       TypeName,
                                       DT,
                                       DK,
                                       Normalized,
                                       Size);
}

RSExportType::ExportClass RSExportConstantArrayType::getClass() const {
  return RSExportType::ExportClassConstantArray;
}

const llvm::Type *RSExportConstantArrayType::convertToLLVMType() const {
  llvm::LLVMContext &C = getRSContext()->getLLVMContext();
  const llvm::Type *typ;
  switch (getType()) {
    case DataTypeFloat32:
    case DataTypeRSMatrix2x2:
    case DataTypeRSMatrix3x3:
    case DataTypeRSMatrix4x4: {
      typ = llvm::Type::getFloatTy(C);
      break;
    }
    case DataTypeFloat64: {
      typ = llvm::Type::getDoubleTy(C);
      break;
    }
    case DataTypeSigned8:
    case DataTypeUnsigned8: {
      typ = llvm::Type::getInt8Ty(C);
      break;
    }
    case DataTypeSigned16:
    case DataTypeUnsigned16:
    case DataTypeUnsigned565:
    case DataTypeUnsigned5551:
    case DataTypeUnsigned4444: {
      typ = llvm::Type::getInt16Ty(C);
      break;
    }
    case DataTypeSigned32:
    case DataTypeUnsigned32: {
      typ = llvm::Type::getInt32Ty(C);
      break;
    }
    case DataTypeSigned64: {
    //case DataTypeUnsigned64:
      typ = llvm::Type::getInt64Ty(C);
      break;
    }
    case DataTypeBool: {
      typ = llvm::Type::getInt1Ty(C);
      break;
    }
    default: {
      assert(false && "Unknown data type");
      break;
    }
  }
  return llvm::ArrayType::get(typ, mNumElement);
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
  /* 7 */ { "double2",    "double3",  "double4" },
  /* 8 */ { "long2",      "long3",    "long4" },
};

llvm::StringRef RSExportVectorType::GetTypeName(const clang::ExtVectorType *EVT) {
  const clang::Type *ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);

  if ((ElementType->getTypeClass() != clang::Type::Builtin))
    return llvm::StringRef();

  const clang::BuiltinType *BT = UNSAFE_CAST_TYPE(clang::BuiltinType,
                                                  ElementType);
  const char **BaseElement = NULL;

  switch (BT->getKind()) {
    // Compiler is smart enough to optimize following *big if branches* since
    // they all become "constant comparison" after macro expansion
#define SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type)       \
    case builtin_type: {                                                \
      if (type == RSExportPrimitiveType::DataTypeSigned8) \
        BaseElement = VectorTypeNameStore[0];                           \
      else if (type == RSExportPrimitiveType::DataTypeUnsigned8) \
        BaseElement = VectorTypeNameStore[1];                           \
      else if (type == RSExportPrimitiveType::DataTypeSigned16) \
        BaseElement = VectorTypeNameStore[2];                           \
      else if (type == RSExportPrimitiveType::DataTypeUnsigned16) \
        BaseElement = VectorTypeNameStore[3];                           \
      else if (type == RSExportPrimitiveType::DataTypeSigned32) \
        BaseElement = VectorTypeNameStore[4];                           \
      else if (type == RSExportPrimitiveType::DataTypeUnsigned32) \
        BaseElement = VectorTypeNameStore[5];                           \
      else if (type == RSExportPrimitiveType::DataTypeFloat32) \
        BaseElement = VectorTypeNameStore[6];                           \
      else if (type == RSExportPrimitiveType::DataTypeFloat64) \
        BaseElement = VectorTypeNameStore[7];                           \
      else if (type == RSExportPrimitiveType::DataTypeSigned64) \
        BaseElement = VectorTypeNameStore[8];                           \
      else if (type == RSExportPrimitiveType::DataTypeBool) \
        BaseElement = VectorTypeNameStore[0];                          \
      break;  \
    }
#include "slang_rs_export_type_support.inc"
    default: {
      return llvm::StringRef();
    }
  }

  if ((BaseElement != NULL) &&
      (EVT->getNumElements() > 1) &&
      (EVT->getNumElements() <= 4))
    return BaseElement[EVT->getNumElements() - 2];
  else
    return llvm::StringRef();
}

RSExportVectorType *RSExportVectorType::Create(RSContext *Context,
                                               const clang::ExtVectorType *EVT,
                                               const llvm::StringRef &TypeName,
                                               DataKind DK,
                                               bool Normalized) {
  assert(EVT != NULL && EVT->getTypeClass() == Type::ExtVector);

  const clang::Type *ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);
  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetDataType(ElementType);

  if (DT != RSExportPrimitiveType::DataTypeUnknown)
    return new RSExportVectorType(Context,
                                  TypeName,
                                  DT,
                                  DK,
                                  Normalized,
                                  EVT->getNumElements());
  else
    fprintf(stderr, "RSExportVectorType::Create : unsupported base element "
                    "type\n");
  return NULL;
}

RSExportType::ExportClass RSExportVectorType::getClass() const {
  return RSExportType::ExportClassVector;
}

const llvm::Type* RSExportVectorType::convertToLLVMType() const {
  const llvm::Type* ElementType = RSExportPrimitiveType::convertToLLVMType();
  return llvm::VectorType::get(ElementType, getNumElement());
}

/**************************** RSExportRecordType ****************************/
RSExportRecordType *RSExportRecordType::Create(RSContext *Context,
                                               const clang::RecordType *RT,
                                               const llvm::StringRef &TypeName,
                                               bool mIsArtificial) {
  assert(RT != NULL && RT->getTypeClass() == Type::Record);

  const clang::RecordDecl *RD = RT->getDecl();
  assert(RD->isStruct());

  RD = RD->getDefinition();
  if (RD == NULL) {
    // TODO: warning: actual struct definition isn't declared in this moudle
    fprintf(stderr, "RSExportRecordType::Create : this struct is not defined "
                    "in this module.");
    return NULL;
  }

  RSExportRecordType *ERT = new RSExportRecordType(Context,
                                                   TypeName,
                                                   RD->
                                                   hasAttr<clang::PackedAttr>(),
                                                   mIsArtificial);
  unsigned int Index = 0;

  for (clang::RecordDecl::field_iterator FI = RD->field_begin(),
           FE = RD->field_end();
       FI != FE;
       FI++) {
#define FAILED_CREATE_FIELD(err)    do {         \
      if (*err)                                                          \
        fprintf(stderr, \
                "RSExportRecordType::Create : failed to create field (%s)\n", \
                err);                                                   \
      delete ERT;                                                       \
      return NULL;                                                      \
    } while (false)

    // FIXME: All fields should be primitive type
    assert((*FI)->getKind() == clang::Decl::Field);
    clang::FieldDecl *FD = *FI;

    // We don't support bit field
    //
    // TODO: allow bitfield with size 8, 16, 32
    if (FD->isBitField())
      FAILED_CREATE_FIELD("bit field is not supported");

    // Type
    RSExportType* ET = RSExportElement::CreateFromDecl(Context, FD);

    if(ET != NULL)
      ERT->mFields.push_back( new Field(ET, FD->getName(), ERT, Index) );
    else
      FAILED_CREATE_FIELD(FD->getName().str().c_str());
#undef FAILED_CREATE_FIELD
  }

  const clang::ASTRecordLayout &ASTRL =
      Context->getASTContext()->getASTRecordLayout(RD);
  ERT->AllocSize =
      (ASTRL.getSize() > ASTRL.getDataSize()) ?
      (ASTRL.getSize() >> 3) :
      (ASTRL.getDataSize() >> 3);

  return ERT;
}

RSExportType::ExportClass RSExportRecordType::getClass() const {
  return RSExportType::ExportClassRecord;
}

const llvm::Type* RSExportRecordType::convertToLLVMType() const {
  std::vector<const llvm::Type*> FieldTypes;

  for (const_field_iterator FI = fields_begin(),
           FE = fields_end();
       FI != FE;
       FI++) {
    const Field *F = *FI;
    const RSExportType *FET = F->getType();

    FieldTypes.push_back(FET->getLLVMType());
  }

  return llvm::StructType::get(getRSContext()->getLLVMContext(),
                               FieldTypes,
                               mIsPacked);
}

/************************* RSExportRecordType::Field *************************/
size_t RSExportRecordType::Field::getOffsetInParent() const {
  // Struct layout obtains below will be cached by LLVM
  const llvm::StructLayout *SL =
      mParent->getRSContext()->getTargetData()->getStructLayout(
          static_cast<const llvm::StructType*>(mParent->getLLVMType())
                                                                );
  return SL->getElementOffset(mIndex);
}
