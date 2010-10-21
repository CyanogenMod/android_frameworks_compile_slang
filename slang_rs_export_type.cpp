/*
 * Copyright 2010, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "slang_rs_export_type.h"

#include <vector>

#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Target/TargetData.h"

#include "clang/AST/RecordLayout.h"

#include "slang_rs_context.h"
#include "slang_rs_export_element.h"

#define CHECK_PARENT_EQUALITY(ParentClass, E) \
  if (!ParentClass::equals(E))                \
    return false;

using namespace slang;

/****************************** RSExportType ******************************/
bool RSExportType::NormalizeType(const clang::Type *&T,
                                 llvm::StringRef &TypeName) {
  llvm::SmallPtrSet<const clang::Type*, 8> SPS =
      llvm::SmallPtrSet<const clang::Type*, 8>();

  if ((T = RSExportType::TypeExportable(T, SPS)) == NULL)
    // TODO(zonr): warn that type not exportable.
    return false;

  // Get type name
  TypeName = RSExportType::GetTypeName(T);
  if (TypeName.empty())
    // TODO(zonr): warning that the type is unnamed.
    return false;

  return true;
}

const clang::Type
*RSExportType::GetTypeOfDecl(const clang::DeclaratorDecl *DD) {
  if (DD) {
    clang::QualType T;
    if (DD->getTypeSourceInfo())
      T = DD->getTypeSourceInfo()->getType();
    else
      T = DD->getType();

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
#define ENUM_SUPPORT_BUILTIN_TYPE(builtin_type, type, cname)  \
        case builtin_type:                                    \
          return cname;                                       \
        break;
#include "RSClangBuiltinEnums.inc"
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

          if (Name.empty())
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
      const clang::Type *PT = GET_POINTEE_TYPE(T);
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
    case clang::Type::ExtVector: {
      const clang::ExtVectorType *EVT =
          UNSAFE_CAST_TYPE(clang::ExtVectorType, T);
      return RSExportVectorType::GetTypeName(EVT);
      break;
    }
    case clang::Type::ConstantArray : {
      // Construct name for a constant array is too complicated.
      return DUMMY_TYPE_NAME_FOR_RS_CONSTANT_ARRAY_TYPE;
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
#define ENUM_SUPPORT_BUILTIN_TYPE(builtin_type, type, cname)  \
        case builtin_type:
#include "RSClangBuiltinEnums.inc"
          return T;
        default: {
          return NULL;
        }
      }
    }
    case clang::Type::Record: {
      if (RSExportPrimitiveType::GetRSObjectType(T) !=
          RSExportPrimitiveType::DataTypeUnknown)
        return T;  // RS object type, no further checks are needed

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
        const clang::FieldDecl *FD = *FI;
        const clang::Type *FT = GetTypeOfDecl(FD);
        FT = GET_CANONICAL_TYPE(FT);

        if (!TypeExportable(FT, SPS)) {
          fprintf(stderr, "Field `%s' in Record `%s' contains unsupported "
                          "type\n", FD->getNameAsString().c_str(),
                                    RD->getNameAsString().c_str());
          FT->dump();
          return NULL;
        }
      }

      return T;
    }
    case clang::Type::Pointer: {
      const clang::PointerType *PT = UNSAFE_CAST_TYPE(clang::PointerType, T);
      const clang::Type *PointeeType = GET_POINTEE_TYPE(PT);

      if (PointeeType->getTypeClass() == clang::Type::Pointer)
        return T;
      // We don't support pointer with array-type pointee or unsupported pointee
      // type
      if (PointeeType->isArrayType() ||
         (TypeExportable(PointeeType, SPS) == NULL) )
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
    case clang::Type::ConstantArray: {
      const clang::ConstantArrayType *CAT =
          UNSAFE_CAST_TYPE(clang::ConstantArrayType, T);

      // Check size
      if (CAT->getSize().getActiveBits() > 32) {
        fprintf(stderr, "RSExportConstantArrayType::Create : array with too "
                        "large size (> 2^32).\n");
        return NULL;
      }
      // Check element type
      const clang::Type *ElementType = GET_CONSTANT_ARRAY_ELEMENT_TYPE(CAT);
      if (ElementType->isArrayType()) {
        fprintf(stderr, "RSExportType::TypeExportable : constant array with 2 "
                        "or higher dimension of constant is not supported.\n");
        return NULL;
      }
      if (TypeExportable(ElementType, SPS) == NULL)
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
        case RSExportPrimitiveType::DataTypeUnknown: {
          // User-defined types
          ET = RSExportRecordType::Create(Context,
                                          T->getAsStructureType(),
                                          TypeName);
          break;
        }
        case RSExportPrimitiveType::DataTypeRSMatrix2x2: {
          // 2 x 2 Matrix type
          ET = RSExportMatrixType::Create(Context,
                                          T->getAsStructureType(),
                                          TypeName,
                                          2);
          break;
        }
        case RSExportPrimitiveType::DataTypeRSMatrix3x3: {
          // 3 x 3 Matrix type
          ET = RSExportMatrixType::Create(Context,
                                          T->getAsStructureType(),
                                          TypeName,
                                          3);
          break;
        }
        case RSExportPrimitiveType::DataTypeRSMatrix4x4: {
          // 4 x 4 Matrix type
          ET = RSExportMatrixType::Create(Context,
                                          T->getAsStructureType(),
                                          TypeName,
                                          4);
          break;
        }
        default: {
          // Others are primitive types
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
      // FIXME: free the name (allocated in RSExportType::GetTypeName)
      delete [] TypeName.data();
      break;
    }
    case clang::Type::ExtVector: {
      ET = RSExportVectorType::Create(Context,
                                      UNSAFE_CAST_TYPE(clang::ExtVectorType, T),
                                      TypeName);
      break;
    }
    case clang::Type::ConstantArray: {
      ET = RSExportConstantArrayType::Create(
              Context,
              UNSAFE_CAST_TYPE(clang::ConstantArrayType, T));
      break;
    }
    default: {
      // TODO(zonr): warn that type is not exportable.
      fprintf(stderr,
              "RSExportType::Create : type '%s' is not exportable\n",
              T->getTypeClassName());
      break;
    }
  }

  return ET;
}

RSExportType *RSExportType::Create(RSContext *Context, const clang::Type *T) {
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
      ET->getLLVMType());
}

size_t RSExportType::GetTypeAllocSize(const RSExportType *ET) {
  if (ET->getClass() == RSExportType::ExportClassRecord)
    return static_cast<const RSExportRecordType*>(ET)->getAllocSize();
  else
    return ET->getRSContext()->getTargetData()->getTypeAllocSize(
        ET->getLLVMType());
}

RSExportType::RSExportType(RSContext *Context,
                           ExportClass Class,
                           const llvm::StringRef &Name)
    : RSExportable(Context, RSExportable::EX_TYPE),
      mClass(Class),
      // Make a copy on Name since memory stored @Name is either allocated in
      // ASTContext or allocated in GetTypeName which will be destroyed later.
      mName(Name.data(), Name.size()),
      mLLVMType(NULL) {
  // Don't cache the type whose name start with '<'. Those type failed to
  // get their name since constructing their name in GetTypeName() requiring
  // complicated work.
  if (!Name.startswith(DUMMY_RS_TYPE_NAME_PREFIX))
    // TODO(zonr): Need to check whether the insertion is successful or not.
    Context->insertExportType(llvm::StringRef(Name), this);
  return;
}

bool RSExportType::keep() {
  if (!RSExportable::keep())
    return false;
  // Invalidate converted LLVM type.
  mLLVMType = NULL;
  return true;
}

bool RSExportType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportable, E);
  return (static_cast<const RSExportType*>(E)->getClass() == getClass());
}

/************************** RSExportPrimitiveType **************************/
llvm::ManagedStatic<RSExportPrimitiveType::RSObjectTypeMapTy>
RSExportPrimitiveType::RSObjectTypeMap;

llvm::Type *RSExportPrimitiveType::RSObjectLLVMType = NULL;

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

  if (RSObjectTypeMap->empty()) {
#define ENUM_RS_OBJECT_TYPE(type, cname)                            \
    RSObjectTypeMap->GetOrCreateValue(cname, DataType ## type);
#include "RSObjectTypeEnums.inc"
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

const size_t RSExportPrimitiveType::SizeOfDataTypeInBits[] = {
#define ENUM_RS_DATA_TYPE(type, cname, bits)  \
  bits,
#include "RSDataTypeEnums.inc"
  0   // DataTypeMax
};

size_t RSExportPrimitiveType::GetSizeInBits(const RSExportPrimitiveType *EPT) {
  assert(((EPT->getType() > DataTypeUnknown) &&
          (EPT->getType() < DataTypeMax)) &&
         "RSExportPrimitiveType::GetSizeInBits : unknown data type");
  return SizeOfDataTypeInBits[ static_cast<int>(EPT->getType()) ];
}

RSExportPrimitiveType::DataType
RSExportPrimitiveType::GetDataType(const clang::Type *T) {
  if (T == NULL)
    return DataTypeUnknown;

  switch (T->getTypeClass()) {
    case clang::Type::Builtin: {
      const clang::BuiltinType *BT = UNSAFE_CAST_TYPE(clang::BuiltinType, T);
      switch (BT->getKind()) {
#define ENUM_SUPPORT_BUILTIN_TYPE(builtin_type, type, cname)  \
        case builtin_type: {                                  \
          return DataType ## type;                            \
        }
#include "RSClangBuiltinEnums.inc"
        // The size of type WChar depend on platform so we abandon the support
        // to them.
        default: {
          fprintf(stderr, "RSExportPrimitiveType::GetDataType : unsupported "
                          "built-in type '%s'\n.", T->getTypeClassName());
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
                      "supported primitive type\n", T->getTypeClassName());
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
    return new RSExportPrimitiveType(Context, ExportClassPrimitive, TypeName,
                                     DT, DK, Normalized);
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
      Elements.push_back(llvm::ArrayType::get(llvm::Type::getInt32Ty(C), 1));
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
    case DataTypeBoolean: {
      return llvm::Type::getInt1Ty(C);
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
    case DataTypeSigned64:
    case DataTypeUnsigned64: {
      return llvm::Type::getInt64Ty(C);
      break;
    }
    default: {
      assert(false && "Unknown data type");
    }
  }

  return NULL;
}

bool RSExportPrimitiveType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportType, E);
  return (static_cast<const RSExportPrimitiveType*>(E)->getType() == getType());
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

const llvm::Type *RSExportPointerType::convertToLLVMType() const {
  const llvm::Type *PointeeType = mPointeeType->getLLVMType();
  return llvm::PointerType::getUnqual(PointeeType);
}

bool RSExportPointerType::keep() {
  if (!RSExportType::keep())
    return false;
  const_cast<RSExportType*>(mPointeeType)->keep();
  return true;
}

bool RSExportPointerType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportType, E);
  return (static_cast<const RSExportPointerType*>(E)
              ->getPointeeType()->equals(getPointeeType()));
}

/***************************** RSExportVectorType *****************************/
llvm::StringRef
RSExportVectorType::GetTypeName(const clang::ExtVectorType *EVT) {
  const clang::Type *ElementType = GET_EXT_VECTOR_ELEMENT_TYPE(EVT);

  if ((ElementType->getTypeClass() != clang::Type::Builtin))
    return llvm::StringRef();

  const clang::BuiltinType *BT = UNSAFE_CAST_TYPE(clang::BuiltinType,
                                                  ElementType);
  if ((EVT->getNumElements() < 1) ||
      (EVT->getNumElements() > 4))
    return llvm::StringRef();

  switch (BT->getKind()) {
    // Compiler is smart enough to optimize following *big if branches* since
    // they all become "constant comparison" after macro expansion
#define ENUM_SUPPORT_BUILTIN_TYPE(builtin_type, type, cname)  \
    case builtin_type: {                                      \
      const char *Name[] = { cname"2", cname"3", cname"4" };  \
      return Name[EVT->getNumElements() - 2];                 \
      break;                                                  \
    }
#include "RSClangBuiltinEnums.inc"
    default: {
      return llvm::StringRef();
    }
  }
}

RSExportVectorType *RSExportVectorType::Create(RSContext *Context,
                                               const clang::ExtVectorType *EVT,
                                               const llvm::StringRef &TypeName,
                                               DataKind DK,
                                               bool Normalized) {
  assert(EVT != NULL && EVT->getTypeClass() == clang::Type::ExtVector);

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

const llvm::Type *RSExportVectorType::convertToLLVMType() const {
  const llvm::Type *ElementType = RSExportPrimitiveType::convertToLLVMType();
  return llvm::VectorType::get(ElementType, getNumElement());
}

bool RSExportVectorType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportPrimitiveType, E);
  return (static_cast<const RSExportVectorType*>(E)->getNumElement()
              == getNumElement());
}

/***************************** RSExportMatrixType *****************************/
RSExportMatrixType *RSExportMatrixType::Create(RSContext *Context,
                                               const clang::RecordType *RT,
                                               const llvm::StringRef &TypeName,
                                               unsigned Dim) {
  assert((RT != NULL) && (RT->getTypeClass() == clang::Type::Record));
  assert((Dim > 1) && "Invalid dimension of matrix");

  // Check whether the struct rs_matrix is in our expected form (but assume it's
  // correct if we're not sure whether it's correct or not)
  const clang::RecordDecl* RD = RT->getDecl();
  RD = RD->getDefinition();
  if (RD != NULL) {
    // Find definition, perform further examination
    if (RD->field_empty()) {
      fprintf(stderr, "RSExportMatrixType::Create : invalid %s struct: "
                      "must have 1 field for saving values", TypeName.data());
      return NULL;
    }

    clang::RecordDecl::field_iterator FIT = RD->field_begin();
    const clang::FieldDecl *FD = *FIT;
    const clang::Type *FT = RSExportType::GetTypeOfDecl(FD);
    if ((FT == NULL) || (FT->getTypeClass() != clang::Type::ConstantArray)) {
      fprintf(stderr, "RSExportMatrixType::Create : invalid %s struct: "
                      "first field should be an array with constant size",
              TypeName.data());
      return NULL;
    }
    const clang::ConstantArrayType *CAT =
      static_cast<const clang::ConstantArrayType *>(FT);
    const clang::Type *ElementType = GET_CONSTANT_ARRAY_ELEMENT_TYPE(CAT);
    if ((ElementType == NULL) ||
        (ElementType->getTypeClass() != clang::Type::Builtin) ||
        (static_cast<const clang::BuiltinType *>(ElementType)->getKind()
          != clang::BuiltinType::Float)) {
      fprintf(stderr, "RSExportMatrixType::Create : invalid %s struct: "
                      "first field should be a float array", TypeName.data());
      return NULL;
    }

    if (CAT->getSize() != Dim * Dim) {
      fprintf(stderr, "RSExportMatrixType::Create : invalid %s struct: "
                      "first field should be an array with size %d",
              TypeName.data(), Dim * Dim);
      return NULL;
    }

    FIT++;
    if (FIT != RD->field_end()) {
      fprintf(stderr, "RSExportMatrixType::Create : invalid %s struct: "
                      "must have exactly 1 field", TypeName.data());
      return NULL;
    }
  }

  return new RSExportMatrixType(Context, TypeName, Dim);
}

const llvm::Type *RSExportMatrixType::convertToLLVMType() const {
  // Construct LLVM type:
  // struct {
  //  float X[mDim * mDim];
  // }

  llvm::LLVMContext &C = getRSContext()->getLLVMContext();
  llvm::ArrayType *X = llvm::ArrayType::get(llvm::Type::getFloatTy(C),
                                            mDim * mDim);
  return llvm::StructType::get(C, X, NULL);
}

bool RSExportMatrixType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportType, E);
  return (static_cast<const RSExportMatrixType*>(E)->getDim() == getDim());
}

/************************* RSExportConstantArrayType *************************/
RSExportConstantArrayType
*RSExportConstantArrayType::Create(RSContext *Context,
                                   const clang::ConstantArrayType *CAT) {
  assert(CAT != NULL && CAT->getTypeClass() == clang::Type::ConstantArray);

  assert((CAT->getSize().getActiveBits() < 32) && "array too large");

  unsigned Size = static_cast<unsigned>(CAT->getSize().getZExtValue());
  assert((Size > 0) && "Constant array should have size greater than 0");

  const clang::Type *ElementType = GET_CONSTANT_ARRAY_ELEMENT_TYPE(CAT);
  RSExportType *ElementET = RSExportType::Create(Context, ElementType);

  if (ElementET == NULL) {
    fprintf(stderr, "RSExportConstantArrayType::Create : failed to create "
                    "RSExportType for array element.\n");
    return NULL;
  }

  return new RSExportConstantArrayType(Context,
                                       ElementET,
                                       Size);
}

const llvm::Type *RSExportConstantArrayType::convertToLLVMType() const {
  return llvm::ArrayType::get(mElementType->getLLVMType(), getSize());
}

bool RSExportConstantArrayType::keep() {
  if (!RSExportType::keep())
    return false;
  const_cast<RSExportType*>(mElementType)->keep();
  return true;
}

bool RSExportConstantArrayType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportType, E);
  return ((static_cast<const RSExportConstantArrayType*>(E)
              ->getSize() == getSize()) && (mElementType->equals(E)));
}

/**************************** RSExportRecordType ****************************/
RSExportRecordType *RSExportRecordType::Create(RSContext *Context,
                                               const clang::RecordType *RT,
                                               const llvm::StringRef &TypeName,
                                               bool mIsArtificial) {
  assert(RT != NULL && RT->getTypeClass() == clang::Type::Record);

  const clang::RecordDecl *RD = RT->getDecl();
  assert(RD->isStruct());

  RD = RD->getDefinition();
  if (RD == NULL) {
    // TODO(zonr): warn that actual struct definition isn't declared in this
    //             moudle.
    fprintf(stderr, "RSExportRecordType::Create : this struct is not defined "
                    "in this module.");
    return NULL;
  }

  // Struct layout construct by clang. We rely on this for obtaining the
  // alloc size of a struct and offset of every field in that struct.
  const clang::ASTRecordLayout *RL =
      &Context->getASTContext()->getASTRecordLayout(RD);
  assert((RL != NULL) && "Failed to retrieve the struct layout from Clang.");

  RSExportRecordType *ERT =
      new RSExportRecordType(Context,
                             TypeName,
                             RD->hasAttr<clang::PackedAttr>(),
                             mIsArtificial,
                             (RL->getSize() >> 3));
  unsigned int Index = 0;

  for (clang::RecordDecl::field_iterator FI = RD->field_begin(),
           FE = RD->field_end();
       FI != FE;
       FI++, Index++) {
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
    // TODO(zonr): allow bitfield with size 8, 16, 32
    if (FD->isBitField())
      FAILED_CREATE_FIELD("bit field is not supported");

    // Type
    RSExportType *ET = RSExportElement::CreateFromDecl(Context, FD);

    if (ET != NULL)
      ERT->mFields.push_back(
          new Field(ET, FD->getName(), ERT,
                    static_cast<size_t>(RL->getFieldOffset(Index) >> 3)));
    else
      FAILED_CREATE_FIELD(FD->getName().str().c_str());
#undef FAILED_CREATE_FIELD
  }

  return ERT;
}

const llvm::Type *RSExportRecordType::convertToLLVMType() const {
  // Create an opaque type since struct may reference itself recursively.
  llvm::PATypeHolder ResultHolder =
      llvm::OpaqueType::get(getRSContext()->getLLVMContext());
  setAbstractLLVMType(ResultHolder.get());

  std::vector<const llvm::Type*> FieldTypes;

  for (const_field_iterator FI = fields_begin(), FE = fields_end();
       FI != FE;
       FI++) {
    const Field *F = *FI;
    const RSExportType *FET = F->getType();

    FieldTypes.push_back(FET->getLLVMType());
  }

  llvm::StructType *ST = llvm::StructType::get(getRSContext()->getLLVMContext(),
                                               FieldTypes,
                                               mIsPacked);
  if (ST != NULL)
    static_cast<llvm::OpaqueType*>(ResultHolder.get())
        ->refineAbstractTypeTo(ST);
  else
    return NULL;
  return ResultHolder.get();
}

bool RSExportRecordType::keep() {
  if (!RSExportType::keep())
    return false;
  for (std::list<const Field*>::iterator I = mFields.begin(),
          E = mFields.end();
       I != E;
       I++) {
    const_cast<RSExportType*>((*I)->getType())->keep();
  }
  return true;
}

bool RSExportRecordType::equals(const RSExportable *E) const {
  CHECK_PARENT_EQUALITY(RSExportType, E);

  const RSExportRecordType *ERT = static_cast<const RSExportRecordType*>(E);

  if (ERT->getFields().size() != getFields().size())
    return false;

  const_field_iterator AI = fields_begin(), BI = ERT->fields_begin();

  for (unsigned i = 0, e = getFields().size(); i != e; i++) {
    if (!(*AI)->getType()->equals((*BI)->getType()))
      return false;
    AI++;
    BI++;
  }

  return true;
}
