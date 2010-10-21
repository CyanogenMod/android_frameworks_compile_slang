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

#include <string>
#include <cstring>
#include <cstdio>

#include "slang_rs_type_spec.h"

enum {
#define ENUM_PRIMITIVE_DATA_TYPE(x, name, bits) x,
  PRIMITIVE_DATA_TYPE_ENUMS
#undef ENUM_PRIMITIVE_DATA_TYPE
#define ENUM_RS_OBJECT_DATA_TYPE(x, name) x,
  RS_OBJECT_DATA_TYPE_ENUMS
#undef ENUM_RS_OBJECT_DATA_TYPE
};

enum {
#define ENUM_RS_DATA_KIND(x) x,
  RS_DATA_KIND_ENUMS
#undef ENUM_RS_DATA_KIND
};

class RSDataTypeSpec {
 private:
  const char *mTypeName;        // e.g. Float32
  // FIXME: better name
  const char *mTypePragmaName;  // e.g. float
  size_t mBits;

 protected:
  bool mIsRSObject;

 public:
  RSDataTypeSpec(const char *TypeName,
                 const char *TypePragmaName,
                 size_t Bits)
      : mTypeName(TypeName),
        mTypePragmaName(TypePragmaName),
        mBits(Bits),
        mIsRSObject(false) {
    return;
  }

  inline const char *getTypeName() const { return mTypeName; }
  inline const char *getTypePragmaName() const { return mTypePragmaName; }
  inline size_t getSizeInBit() const { return mBits; }
  inline bool isRSObject() const { return mIsRSObject; }
};

class RSObjectDataTypeSpec : public RSDataTypeSpec {
 public:
  RSObjectDataTypeSpec(const char *TypeName,
                       const char *TypePragmaName)
      : RSDataTypeSpec(TypeName, TypePragmaName, 32 /* opaque pointer */) {
    mIsRSObject = true;
    return;
  }
};

/////////////////////////////////////////////////////////////////////////////

// clang::BuiltinType::Kind -> RSDataTypeSpec
class ClangBuiltinTypeMap {
  const char *mBuiltinTypeKind;
  const RSDataTypeSpec *mDataType;

 public:
  ClangBuiltinTypeMap(const char *BuiltinTypeKind,
                      const RSDataTypeSpec *DataType)
      : mBuiltinTypeKind(BuiltinTypeKind),
        mDataType(DataType) {
    return;
  }

  inline const char *getBuiltinTypeKind() const { return mBuiltinTypeKind; }
  inline const RSDataTypeSpec *getDataType() const { return mDataType; }
};

/////////////////////////////////////////////////////////////////////////////

class RSDataKindSpec {
 private:
  const char *mKindName;

 public:
  explicit RSDataKindSpec(const char *KindName) : mKindName(KindName) {
    return;
  }

  inline const char *getKindName() const { return mKindName; }
};

/////////////////////////////////////////////////////////////////////////////

class RSDataElementSpec {
 private:
  const char *mElementName;
  const RSDataKindSpec *mDataKind;
  const RSDataTypeSpec *mDataType;
  bool mIsNormal;
  unsigned mVectorSize;

 public:
  RSDataElementSpec(const char *ElementName,
                    const RSDataKindSpec *DataKind,
                    const RSDataTypeSpec *DataType,
                    bool IsNormal,
                    unsigned VectorSize)
      : mElementName(ElementName),
        mDataKind(DataKind),
        mDataType(DataType),
        mIsNormal(IsNormal),
        mVectorSize(VectorSize) {
    return;
  }

  inline const char *getElementName() const { return mElementName; }
  inline const RSDataKindSpec *getDataKind() const { return mDataKind; }
  inline const RSDataTypeSpec *getDataType() const { return mDataType; }
  inline bool isNormal() const { return mIsNormal; }
  inline unsigned getVectorSize() const { return mVectorSize; }
};

/////////////////////////////////////////////////////////////////////////////

// -gen-rs-data-type-enums
//
// ENUM_RS_DATA_TYPE(type, cname, bits)
// e.g., ENUM_RS_DATA_TYPE(Float32, "float", 256)
static int GenRSDataTypeEnums(const RSDataTypeSpec *const DataTypes[],
                              unsigned NumDataTypes) {
  for (unsigned i = 0; i < NumDataTypes; i++)
    printf("ENUM_RS_DATA_TYPE(%s, \"%s\", %lu)\n",
           DataTypes[i]->getTypeName(),
           DataTypes[i]->getTypePragmaName(),
           DataTypes[i]->getSizeInBit());
  printf("#undef ENUM_RS_DATA_TYPE");
  return 0;
}

// -gen-clang-builtin-cnames
//
// ENUM_SUPPORT_BUILTIN_TYPE(builtin_type, type, cname)
// e.g., ENUM_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Float, Float32, "float")
static int GenClangBuiltinEnum(
    const ClangBuiltinTypeMap *const ClangBuilitinsMap[],
    unsigned NumClangBuilitins) {
  for (unsigned i = 0; i < NumClangBuilitins; i++)
    printf("ENUM_SUPPORT_BUILTIN_TYPE(%s, %s, \"%s\")\n",
           ClangBuilitinsMap[i]->getBuiltinTypeKind(),
           ClangBuilitinsMap[i]->getDataType()->getTypeName(),
           ClangBuilitinsMap[i]->getDataType()->getTypePragmaName());
  printf("#undef ENUM_SUPPORT_BUILTIN_TYPE");
  return 0;
}

// -gen-rs-object-type-enums
//
// ENUM_RS_OBJECT_TYPE(type, cname)
// e.g., ENUM_RS_OBJECT_TYPE(RSMatrix2x2, "rs_matrix2x2")
static int GenRSObjectTypeEnums(const RSDataTypeSpec *const DataTypes[],
                                unsigned NumDataTypes) {
  for (unsigned i = 0; i < NumDataTypes; i++)
    if (DataTypes[i]->isRSObject())
      printf("ENUM_RS_OBJECT_TYPE(%s, \"%s\")\n",
             DataTypes[i]->getTypeName(),
             DataTypes[i]->getTypePragmaName());
  printf("#undef ENUM_RS_OBJECT_TYPE");
  return 0;
}

// -gen-rs-data-kind-enums
//
// ENUM_RS_DATA_KIND(kind)
// e.g., ENUM_RS_DATA_KIND(PixelRGB)
int GenRSDataKindEnums(const RSDataKindSpec *const DataKinds[],
                       unsigned NumDataKinds) {
  for (unsigned i = 0; i < NumDataKinds; i++)
    printf("ENUM_RS_DATA_KIND(%s)\n", DataKinds[i]->getKindName());
  printf("#undef ENUM_RS_DATA_KIND");
  return 0;
}

// -gen-rs-data-element-enums
//
// ENUM_RS_DATA_ELEMENT(name, dt, dk, normailized, vsize)
// e.g., ENUM_RS_DATA_ELEMENT("rs_pixel_rgba", PixelRGB, Unsigned8, true, 4)
int GenRSDataElementEnums(const RSDataElementSpec *const DataElements[],
                          unsigned NumDataElements) {
  for (unsigned i = 0; i < NumDataElements; i++)
    printf("ENUM_RS_DATA_ELEMENT(\"%s\", %s, %s, %s, %d)\n",
           DataElements[i]->getElementName(),
           DataElements[i]->getDataKind()->getKindName(),
           DataElements[i]->getDataType()->getTypeName(),
           ((DataElements[i]->isNormal()) ? "true" : "false"),
           DataElements[i]->getVectorSize());
  printf("#undef ENUM_RS_DATA_ELEMENT");
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s [gen type]\n", argv[0]);
    return 1;
  }

  RSDataTypeSpec *DataTypes[] = {
#define ENUM_PRIMITIVE_DATA_TYPE(x, name, bits) \
  new RSDataTypeSpec(#x , name, bits),
  PRIMITIVE_DATA_TYPE_ENUMS
#undef ENUM_PRIMITIVE_DATA_TYPE
#define ENUM_RS_OBJECT_DATA_TYPE(x, name)  \
  new RSObjectDataTypeSpec(#x, name),
  RS_OBJECT_DATA_TYPE_ENUMS
#undef ENUM_RS_OBJECT_DATA_TYPE
  };

  unsigned NumDataTypes = sizeof(DataTypes) / sizeof(DataTypes[0]);
  /////////////////////////////////////////////////////////////////////////////

  ClangBuiltinTypeMap *ClangBuilitinsMap[] = {
    new ClangBuiltinTypeMap("clang::BuiltinType::Bool",   DataTypes[Boolean]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Char_U", DataTypes[Unsigned8]),
    new ClangBuiltinTypeMap("clang::BuiltinType::UChar",  DataTypes[Unsigned8]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Char16", DataTypes[Signed16]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Char32", DataTypes[Signed32]),
    new ClangBuiltinTypeMap("clang::BuiltinType::UShort", DataTypes[Unsigned16]),
    new ClangBuiltinTypeMap("clang::BuiltinType::UInt",   DataTypes[Unsigned32]),
    new ClangBuiltinTypeMap("clang::BuiltinType::ULong",  DataTypes[Unsigned32]),
    new ClangBuiltinTypeMap("clang::BuiltinType::ULongLong", DataTypes[Unsigned64]),

    new ClangBuiltinTypeMap("clang::BuiltinType::Char_S", DataTypes[Signed8]),
    new ClangBuiltinTypeMap("clang::BuiltinType::SChar",  DataTypes[Signed8]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Short",  DataTypes[Signed16]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Int",    DataTypes[Signed32]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Long",   DataTypes[Signed64]),
    new ClangBuiltinTypeMap("clang::BuiltinType::LongLong", DataTypes[Signed64]),

    new ClangBuiltinTypeMap("clang::BuiltinType::Float",  DataTypes[Float32]),
    new ClangBuiltinTypeMap("clang::BuiltinType::Double", DataTypes[Float64]),
  };

  unsigned NumClangBuilitins =
      sizeof(ClangBuilitinsMap) / sizeof(ClangBuilitinsMap[0]);

  /////////////////////////////////////////////////////////////////////////////

  RSDataKindSpec *DataKinds[] = {
#define ENUM_RS_DATA_KIND(x)  \
    new RSDataKindSpec(#x),
    RS_DATA_KIND_ENUMS
#undef ENUM_RS_DATA_KIND
  };

  unsigned NumDataKinds = sizeof(DataKinds) / sizeof(DataKinds[0]);
  /////////////////////////////////////////////////////////////////////////////

  RSDataElementSpec *DataElements[] = {
    new RSDataElementSpec("rs_pixel_l",
                          DataKinds[PixelL],
                          DataTypes[Unsigned8],
                          /* IsNormal = */true, /* VectorSize = */1),
    new RSDataElementSpec("rs_pixel_a",
                          DataKinds[PixelA],
                          DataTypes[Unsigned8],
                          true, 1),
    new RSDataElementSpec("rs_pixel_la",
                          DataKinds[PixelLA],
                          DataTypes[Unsigned8],
                          true, 2),
    new RSDataElementSpec("rs_pixel_rgb",
                          DataKinds[PixelRGB],
                          DataTypes[Unsigned8],
                          true, 3),
    new RSDataElementSpec("rs_pixel_rgba",
                          DataKinds[PixelRGB],
                          DataTypes[Unsigned8],
                          true, 4),
    new RSDataElementSpec("rs_pixel_rgb565",
                          DataKinds[PixelRGB],
                          DataTypes[Unsigned8],
                          true, 3),
    new RSDataElementSpec("rs_pixel_rgb5551",
                          DataKinds[PixelRGBA],
                          DataTypes[Unsigned8],
                          true, 4),
    new RSDataElementSpec("rs_pixel_rgb4444",
                          DataKinds[PixelRGBA],
                          DataTypes[Unsigned8],
                          true, 4),
  };

  unsigned NumDataElements = sizeof(DataElements) / sizeof(DataElements[0]);
  /////////////////////////////////////////////////////////////////////////////
  int Result = 1;

  if (::strcmp(argv[1], "-gen-rs-data-type-enums") == 0)
    Result = GenRSDataTypeEnums(DataTypes, NumDataTypes);
  else if (::strcmp(argv[1], "-gen-clang-builtin-enums") == 0)
    Result = GenClangBuiltinEnum(ClangBuilitinsMap, NumClangBuilitins);
  else if (::strcmp(argv[1], "-gen-rs-object-type-enums") == 0)
    Result = GenRSObjectTypeEnums(DataTypes, NumDataTypes);
  else if (::strcmp(argv[1], "-gen-rs-data-kind-enums") == 0)
    Result = GenRSDataKindEnums(DataKinds, NumDataKinds);
  else if (::strcmp(argv[1], "-gen-rs-data-element-enums") == 0)
    Result = GenRSDataElementEnums(DataElements, NumDataElements);
  else
    fprintf(stderr, "%s: Unknown table generation type '%s'\n",
                    argv[0], argv[1]);


  /////////////////////////////////////////////////////////////////////////////
  for (unsigned i = 0; i < NumDataTypes; i++)
    delete DataTypes[i];
  for (unsigned i = 0; i < NumClangBuilitins; i++)
    delete ClangBuilitinsMap[i];
  for (unsigned i = 0; i < NumDataKinds; i++)
    delete DataKinds[i];
  for (unsigned i = 0; i < NumDataElements; i++)
    delete DataElements[i];

  return Result;
}
