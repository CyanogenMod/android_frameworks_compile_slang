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

#ifndef _COMPILE_SLANG_SLANG_RS_TYPE_SPEC_H_  // NOLINT
#define _COMPILE_SLANG_SLANG_RS_TYPE_SPEC_H_

enum RSTypeClass {
  RS_TC_Primitive,
  RS_TC_Pointer,
  RS_TC_Vector,
  RS_TC_Matrix,
  RS_TC_ConstantArray,
  RS_TC_Record,
  RS_TC_Max
};

// TODO We have the same enum in PrimitiveDataType.  Join them.
enum RSDataType {
  RS_DT_Float16 = 0,
  RS_DT_Float32 = 1,
  RS_DT_Float64 = 2,
  RS_DT_Signed8 = 3,
  RS_DT_Signed16 = 4,
  RS_DT_Signed32 = 5,
  RS_DT_Signed64 = 6,
  RS_DT_Unsigned8 = 7,
  RS_DT_Unsigned16 = 8,
  RS_DT_Unsigned32 = 9,
  RS_DT_Unsigned64 = 10,
  RS_DT_Boolean = 11,
  RS_DT_Unsigned565 = 12,
  RS_DT_Unsigned5551 = 13,
  RS_DT_Unsigned4444 = 14,
  RS_DT_RSMatrix2x2 = 15,
  RS_DT_RSMatrix3x3 = 16,
  RS_DT_RSMatrix4x4 = 17,
  RS_DT_RSElement = 18,
  RS_DT_RSType = 19,
  RS_DT_RSAllocation = 20,
  RS_DT_RSSampler = 21,
  RS_DT_RSScript = 22,
  RS_DT_RSMesh = 23,
  RS_DT_RSPath = 24,
  RS_DT_RSProgramFragment = 25,
  RS_DT_RSProgramVertex = 26,
  RS_DT_RSProgramRaster = 27,
  RS_DT_RSProgramStore = 28,
  RS_DT_RSFont = 29,
  RS_DT_USER_DEFINED
};

// Forward declaration
union RSType;

// NOTE: Current design need to keep struct RSTypeBase as a 4-byte integer for
//       efficient decoding process (see DecodeTypeMetadata).
struct RSTypeBase {
  /* enum RSTypeClass tc; */
  // tc is encoded in b[0].
  union {
    // FIXME: handle big-endianess case
    unsigned bits;  // NOTE: Little-endian is assumed.
    unsigned char b[4];
  };
};

struct RSPrimitiveType {
  struct RSTypeBase base;
  /* enum RSDataType dt; */
  // dt is encoded in base.b[1]
};

struct RSPointerType {
  struct RSTypeBase base;
  const union RSType *pointee;
};

struct RSVectorType {
  struct RSPrimitiveType base;  // base type of vec must be in primitive type
  /* unsigned char vsize; */
  // vsize is encoded in base.b[2]
};

// RSMatrixType is actually a specialize class of RSPrimitiveType whose value of
// dt (data type) can only be RS_DT_RSMatrix2x2, RS_DT_RSMatrix3x3 and
// RS_DT_RSMatrix4x4.
struct RSMatrixType {
  struct RSTypeBase base;
};

struct RSConstantArrayType {
  struct RSTypeBase base;
  const union RSType *element_type;
  /* unsigned esize; */
  // esize is encoded in base.bits{8-31} in little-endian way. This implicates
  // the number of elements in any constant array type should never exceed 2^24.
};

struct RSRecordField {
  const char *name;  // field name
  const union RSType *type;
};

struct RSRecordType {
  struct RSTypeBase base;
  const char *name;  // type name
  /* unsigned num_fields; */
  // num_fields is encoded in base.bits{16-31} in little-endian way. This
  // implicates the number of fields defined in any record type should never
  // exceed 2^16.

  struct RSRecordField field[1];
};

union RSType {
  struct RSTypeBase base;
  struct RSPrimitiveType prim;
  struct RSPointerType pointer;
  struct RSVectorType vec;
  struct RSConstantArrayType ca;
  struct RSRecordType rec;
};

#define RS_GET_TYPE_BASE(R)               (&((R)->base))
#define RS_CAST_TO_PRIMITIVE_TYPE(R)      (&((R)->prim))
#define RS_CAST_TO_POINTER_TYPE(R)        (&((R)->pointer))
#define RS_CAST_TO_VECTOR_TYPE(R)         (&((R)->vec))
#define RS_CAST_TO_CONSTANT_ARRAY_TYPE(R) (&((R)->ca))
#define RS_CAST_TO_RECORD_TYPE(R)         (&((R)->rec))

// RSType
#define RS_TYPE_GET_CLASS(R)  RS_GET_TYPE_BASE(R)->b[0]
#define RS_TYPE_SET_CLASS(R, V) RS_TYPE_GET_CLASS(R) = (V)

// RSPrimitiveType
#define RS_PRIMITIVE_TYPE_GET_DATA_TYPE(R)  \
    RS_CAST_TO_PRIMITIVE_TYPE(R)->base.b[1]
#define RS_PRIMITIVE_TYPE_SET_DATA_TYPE(R, V) \
    RS_PRIMITIVE_TYPE_GET_DATA_TYPE(R) = (V)

// RSPointerType
#define RS_POINTER_TYPE_GET_POINTEE_TYPE(R) \
    RS_CAST_TO_POINTER_TYPE(R)->pointee
#define RS_POINTER_TYPE_SET_POINTEE_TYPE(R, V) \
    RS_POINTER_TYPE_GET_POINTEE_TYPE(R) = (V)

// RSVectorType
#define RS_VECTOR_TYPE_GET_ELEMENT_TYPE(R) \
    RS_PRIMITIVE_TYPE_GET_DATA_TYPE(R)
#define RS_VECTOR_TYPE_SET_ELEMENT_TYPE(R, V) \
    RS_VECTOR_TYPE_GET_ELEMENT_TYPE(R) = (V)

#define RS_VECTOR_TYPE_GET_VECTOR_SIZE(R) \
    RS_CAST_TO_VECTOR_TYPE(R)->base.base.b[2]
#define RS_VECTOR_TYPE_SET_VECTOR_SIZE(R, V) \
    RS_VECTOR_TYPE_GET_VECTOR_SIZE(R) = (V)

// RSMatrixType
#define RS_MATRIX_TYPE_GET_DATA_TYPE(R) RS_PRIMITIVE_TYPE_GET_DATA_TYPE(R)
#define RS_MATRIX_TYPE_SET_DATA_TYPE(R, V)  \
    RS_MATRIX_TYPE_GET_DATA_TYPE(R) = (V)

// RSConstantArrayType
#define RS_CONSTANT_ARRAY_TYPE_GET_ELEMENT_TYPE(R) \
    RS_CAST_TO_CONSTANT_ARRAY_TYPE(R)->element_type
#define RS_CONSTANT_ARRAY_TYPE_SET_ELEMENT_TYPE(R, V) \
    RS_CONSTANT_ARRAY_TYPE_GET_ELEMENT_TYPE(R) = (V)

#define RS_CONSTANT_ARRAY_TYPE_GET_ELEMENT_SIZE(R)  \
    (RS_CAST_TO_CONSTANT_ARRAY_TYPE(R)->base.bits & 0x00ffffff)
#define RS_CONSTANT_ARRAY_TYPE_SET_ELEMENT_SIZE(R, V) \
    RS_CAST_TO_CONSTANT_ARRAY_TYPE(R)->base.bits =  \
    ((RS_CAST_TO_CONSTANT_ARRAY_TYPE(R)->base.bits & 0x000000ff) |  \
     ((V & 0xffffff) << 8))

// RSRecordType
#define RS_RECORD_TYPE_GET_NAME(R)  RS_CAST_TO_RECORD_TYPE(R)->name
#define RS_RECORD_TYPE_SET_NAME(R, V) RS_RECORD_TYPE_GET_NAME(R) = (V)

#define RS_RECORD_TYPE_GET_NUM_FIELDS(R)  \
    ((RS_CAST_TO_RECORD_TYPE(R)->base.bits & 0xffff0000) >> 16)
#define RS_RECORD_TYPE_SET_NUM_FIELDS(R, V) \
    RS_CAST_TO_RECORD_TYPE(R)->base.bits =  \
    ((RS_CAST_TO_RECORD_TYPE(R)->base.bits & 0x0000ffff) | ((V & 0xffff) << 16))

#define RS_RECORD_TYPE_GET_FIELD_NAME(R, I) \
    RS_CAST_TO_RECORD_TYPE(R)->field[(I)].name
#define RS_RECORD_TYPE_SET_FIELD_NAME(R, I, V) \
    RS_RECORD_TYPE_GET_FIELD_NAME(R, I) = (V)

#define RS_RECORD_TYPE_GET_FIELD_TYPE(R, I) \
    RS_CAST_TO_RECORD_TYPE(R)->field[(I)].type
#define RS_RECORD_TYPE_SET_FIELD_TYPE(R, I, V) \
    RS_RECORD_TYPE_GET_FIELD_TYPE(R, I) = (V)

#endif  // _COMPILE_SLANG_SLANG_RS_TYPE_SPEC_H_  NOLINT
