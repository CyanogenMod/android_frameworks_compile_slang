/*
 * Copyright 2010-2011, The Android Open Source Project
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

#include "slang_rs_reflection.h"

#include <sys/stat.h>

#include <cstdarg>
#include <cctype>

#include <algorithm>
#include <string>
#include <utility>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringExtras.h"

#include "os_sep.h"
#include "slang_rs_context.h"
#include "slang_rs_export_var.h"
#include "slang_rs_export_foreach.h"
#include "slang_rs_export_func.h"
#include "slang_rs_reflect_utils.h"
#include "slang_version.h"
#include "slang_utils.h"

#define RS_SCRIPT_CLASS_NAME_PREFIX      "ScriptC_"
#define RS_SCRIPT_CLASS_SUPER_CLASS_NAME "ScriptC"

#define RS_TYPE_CLASS_NAME_PREFIX        "ScriptField_"
#define RS_TYPE_CLASS_SUPER_CLASS_NAME   "android.renderscript.Script.FieldBase"

#define RS_TYPE_ITEM_CLASS_NAME          "Item"

#define RS_TYPE_ITEM_BUFFER_NAME         "mItemArray"
#define RS_TYPE_ITEM_BUFFER_PACKER_NAME  "mIOBuffer"

#define RS_EXPORT_VAR_INDEX_PREFIX       "mExportVarIdx_"
#define RS_EXPORT_VAR_PREFIX             "mExportVar_"

#define RS_EXPORT_FUNC_INDEX_PREFIX      "mExportFuncIdx_"
#define RS_EXPORT_FOREACH_INDEX_PREFIX   "mExportForEachIdx_"

#define RS_EXPORT_VAR_ALLOCATION_PREFIX  "mAlloction_"
#define RS_EXPORT_VAR_DATA_STORAGE_PREFIX "mData_"

namespace slang {

// Some utility function using internal in RSReflection
static bool GetClassNameFromFileName(const std::string &FileName,
                                     std::string &ClassName) {
  ClassName.clear();

  if (FileName.empty() || (FileName == "-"))
    return true;

  ClassName =
      RSSlangReflectUtils::JavaClassNameFromRSFileName(FileName.c_str());

  return true;
}

static const char *GetPrimitiveTypeName(const RSExportPrimitiveType *EPT) {
  static const char *PrimitiveTypeJavaNameMap[] = {
    "",         // RSExportPrimitiveType::DataTypeFloat16
    "float",    // RSExportPrimitiveType::DataTypeFloat32
    "double",   // RSExportPrimitiveType::DataTypeFloat64
    "byte",     // RSExportPrimitiveType::DataTypeSigned8
    "short",    // RSExportPrimitiveType::DataTypeSigned16
    "int",      // RSExportPrimitiveType::DataTypeSigned32
    "long",     // RSExportPrimitiveType::DataTypeSigned64
    "short",    // RSExportPrimitiveType::DataTypeUnsigned8
    "int",      // RSExportPrimitiveType::DataTypeUnsigned16
    "long",     // RSExportPrimitiveType::DataTypeUnsigned32
    "long",     // RSExportPrimitiveType::DataTypeUnsigned64
    "boolean",  // RSExportPrimitiveType::DataTypeBoolean

    "int",      // RSExportPrimitiveType::DataTypeUnsigned565
    "int",      // RSExportPrimitiveType::DataTypeUnsigned5551
    "int",      // RSExportPrimitiveType::DataTypeUnsigned4444

    "",     // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix2x2
    "",     // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix3x3
    "",     // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix4x4

    "Element",      // RSExportPrimitiveType::DataTypeRSElement
    "Type",         // RSExportPrimitiveType::DataTypeRSType
    "Allocation",   // RSExportPrimitiveType::DataTypeRSAllocation
    "Sampler",      // RSExportPrimitiveType::DataTypeRSSampler
    "Script",       // RSExportPrimitiveType::DataTypeRSScript
    "Mesh",         // RSExportPrimitiveType::DataTypeRSMesh
    "ProgramFragment",  // RSExportPrimitiveType::DataTypeRSProgramFragment
    "ProgramVertex",    // RSExportPrimitiveType::DataTypeRSProgramVertex
    "ProgramRaster",    // RSExportPrimitiveType::DataTypeRSProgramRaster
    "ProgramStore",     // RSExportPrimitiveType::DataTypeRSProgramStore
    "Font",     // RSExportPrimitiveType::DataTypeRSFont
  };
  unsigned TypeId = EPT->getType();

  if (TypeId < (sizeof(PrimitiveTypeJavaNameMap) / sizeof(const char*))) {
    return PrimitiveTypeJavaNameMap[ EPT->getType() ];
  }

  slangAssert(false && "GetPrimitiveTypeName : Unknown primitive data type");
  return NULL;
}

static const char *GetVectorTypeName(const RSExportVectorType *EVT) {
  static const char *VectorTypeJavaNameMap[][3] = {
    /* 0 */ { "Byte2",    "Byte3",    "Byte4" },
    /* 1 */ { "Short2",   "Short3",   "Short4" },
    /* 2 */ { "Int2",     "Int3",     "Int4" },
    /* 3 */ { "Long2",    "Long3",    "Long4" },
    /* 4 */ { "Float2",   "Float3",   "Float4" },
    /* 5 */ { "Double2",  "Double3",  "Double4" }
  };

  const char **BaseElement = NULL;

  switch (EVT->getType()) {
    case RSExportPrimitiveType::DataTypeSigned8: {
      BaseElement = VectorTypeJavaNameMap[0];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned16:
    case RSExportPrimitiveType::DataTypeUnsigned8: {
      BaseElement = VectorTypeJavaNameMap[1];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned32:
    case RSExportPrimitiveType::DataTypeUnsigned16: {
      BaseElement = VectorTypeJavaNameMap[2];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned64:
    case RSExportPrimitiveType::DataTypeUnsigned64:
    case RSExportPrimitiveType::DataTypeUnsigned32: {
      BaseElement = VectorTypeJavaNameMap[3];
      break;
    }
    case RSExportPrimitiveType::DataTypeFloat32: {
      BaseElement = VectorTypeJavaNameMap[4];
      break;
    }
    case RSExportPrimitiveType::DataTypeFloat64: {
      BaseElement = VectorTypeJavaNameMap[5];
      break;
    }
    default: {
      slangAssert(false && "RSReflection::GetVectorTypeName : Unsupported "
                           "vector element data type");
      break;
    }
  }

  slangAssert((EVT->getNumElement() > 1) &&
              (EVT->getNumElement() <= 4) &&
              "Number of elements in vector type is invalid");

  return BaseElement[EVT->getNumElement() - 2];
}

static const char *GetVectorElementName(const RSExportVectorType *EVT) {
  static const char *VectorElementNameMap[][3] = {
    /* 0 */ { "U8_2",   "U8_3",   "U8_4" },
    /* 1 */ { "I8_2",   "I8_3",   "I8_4" },
    /* 2 */ { "U16_2",  "U16_3",  "U16_4" },
    /* 3 */ { "I16_2",  "I16_3",  "I16_4" },
    /* 4 */ { "U32_2",  "U32_3",  "U32_4" },
    /* 5 */ { "I32_2",  "I32_3",  "I32_4" },
    /* 6 */ { "U64_2",  "U64_3",  "U64_4" },
    /* 7 */ { "I64_2",  "I64_3",  "I64_4" },
    /* 8 */ { "F32_2",  "F32_3",  "F32_4" },
    /* 9 */ { "F64_2",  "F64_3",  "F64_4" },
  };

  const char **BaseElement = NULL;

  switch (EVT->getType()) {
    case RSExportPrimitiveType::DataTypeUnsigned8: {
      BaseElement = VectorElementNameMap[0];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned8: {
      BaseElement = VectorElementNameMap[1];
      break;
    }
    case RSExportPrimitiveType::DataTypeUnsigned16: {
      BaseElement = VectorElementNameMap[2];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned16: {
      BaseElement = VectorElementNameMap[3];
      break;
    }
    case RSExportPrimitiveType::DataTypeUnsigned32: {
      BaseElement = VectorElementNameMap[4];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned32: {
      BaseElement = VectorElementNameMap[5];
      break;
    }
    case RSExportPrimitiveType::DataTypeUnsigned64: {
      BaseElement = VectorElementNameMap[6];
      break;
    }
    case RSExportPrimitiveType::DataTypeSigned64: {
      BaseElement = VectorElementNameMap[7];
      break;
    }
    case RSExportPrimitiveType::DataTypeFloat32: {
      BaseElement = VectorElementNameMap[8];
      break;
    }
    case RSExportPrimitiveType::DataTypeFloat64: {
      BaseElement = VectorElementNameMap[9];
      break;
    }
    default: {
      slangAssert(false && "RSReflection::GetVectorElementName : Unsupported "
                           "vector element data type");
      break;
    }
  }

  slangAssert((EVT->getNumElement() > 1) &&
              (EVT->getNumElement() <= 4) &&
              "Number of elements in vector type is invalid");

  return BaseElement[EVT->getNumElement() - 2];
}

static const char *GetMatrixTypeName(const RSExportMatrixType *EMT) {
  static const char *MatrixTypeJavaNameMap[] = {
    /* 2x2 */ "Matrix2f",
    /* 3x3 */ "Matrix3f",
    /* 4x4 */ "Matrix4f",
  };
  unsigned Dim = EMT->getDim();

  if ((Dim - 2) < (sizeof(MatrixTypeJavaNameMap) / sizeof(const char*)))
    return MatrixTypeJavaNameMap[ EMT->getDim() - 2 ];

  slangAssert(false && "GetMatrixTypeName : Unsupported matrix dimension");
  return NULL;
}

static const char *GetVectorAccessor(unsigned Index) {
  static const char *VectorAccessorMap[] = {
    /* 0 */ "x",
    /* 1 */ "y",
    /* 2 */ "z",
    /* 3 */ "w",
  };

  slangAssert((Index < (sizeof(VectorAccessorMap) / sizeof(const char*))) &&
              "Out-of-bound index to access vector member");

  return VectorAccessorMap[Index];
}

static const char *GetPackerAPIName(const RSExportPrimitiveType *EPT) {
  static const char *PrimitiveTypePackerAPINameMap[] = {
    "",         // RSExportPrimitiveType::DataTypeFloat16
    "addF32",   // RSExportPrimitiveType::DataTypeFloat32
    "addF64",   // RSExportPrimitiveType::DataTypeFloat64
    "addI8",    // RSExportPrimitiveType::DataTypeSigned8
    "addI16",   // RSExportPrimitiveType::DataTypeSigned16
    "addI32",   // RSExportPrimitiveType::DataTypeSigned32
    "addI64",   // RSExportPrimitiveType::DataTypeSigned64
    "addU8",    // RSExportPrimitiveType::DataTypeUnsigned8
    "addU16",   // RSExportPrimitiveType::DataTypeUnsigned16
    "addU32",   // RSExportPrimitiveType::DataTypeUnsigned32
    "addU64",   // RSExportPrimitiveType::DataTypeUnsigned64
    "addBoolean",  // RSExportPrimitiveType::DataTypeBoolean

    "addU16",   // RSExportPrimitiveType::DataTypeUnsigned565
    "addU16",   // RSExportPrimitiveType::DataTypeUnsigned5551
    "addU16",   // RSExportPrimitiveType::DataTypeUnsigned4444

    "addMatrix",   // RSExportPrimitiveType::DataTypeRSMatrix2x2
    "addMatrix",   // RSExportPrimitiveType::DataTypeRSMatrix3x3
    "addMatrix",   // RSExportPrimitiveType::DataTypeRSMatrix4x4

    "addObj",   // RSExportPrimitiveType::DataTypeRSElement
    "addObj",   // RSExportPrimitiveType::DataTypeRSType
    "addObj",   // RSExportPrimitiveType::DataTypeRSAllocation
    "addObj",   // RSExportPrimitiveType::DataTypeRSSampler
    "addObj",   // RSExportPrimitiveType::DataTypeRSScript
    "addObj",   // RSExportPrimitiveType::DataTypeRSMesh
    "addObj",   // RSExportPrimitiveType::DataTypeRSProgramFragment
    "addObj",   // RSExportPrimitiveType::DataTypeRSProgramVertex
    "addObj",   // RSExportPrimitiveType::DataTypeRSProgramRaster
    "addObj",   // RSExportPrimitiveType::DataTypeRSProgramStore
    "addObj",   // RSExportPrimitiveType::DataTypeRSFont
  };
  unsigned TypeId = EPT->getType();

  if (TypeId < (sizeof(PrimitiveTypePackerAPINameMap) / sizeof(const char*)))
    return PrimitiveTypePackerAPINameMap[ EPT->getType() ];

  slangAssert(false && "GetPackerAPIName : Unknown primitive data type");
  return NULL;
}

static std::string GetTypeName(const RSExportType *ET) {
  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive: {
      return GetPrimitiveTypeName(
                static_cast<const RSExportPrimitiveType*>(ET));
    }
    case RSExportType::ExportClassPointer: {
      const RSExportType *PointeeType =
          static_cast<const RSExportPointerType*>(ET)->getPointeeType();

      if (PointeeType->getClass() != RSExportType::ExportClassRecord)
        return "Allocation";
      else
        return RS_TYPE_CLASS_NAME_PREFIX + PointeeType->getName();
    }
    case RSExportType::ExportClassVector: {
      return GetVectorTypeName(static_cast<const RSExportVectorType*>(ET));
    }
    case RSExportType::ExportClassMatrix: {
      return GetMatrixTypeName(static_cast<const RSExportMatrixType*>(ET));
    }
    case RSExportType::ExportClassConstantArray: {
      const RSExportConstantArrayType* CAT =
          static_cast<const RSExportConstantArrayType*>(ET);
      std::string ElementTypeName = GetTypeName(CAT->getElementType());
      ElementTypeName.append("[]");
      return ElementTypeName;
    }
    case RSExportType::ExportClassRecord: {
      return RS_TYPE_CLASS_NAME_PREFIX + ET->getName() +
                 "."RS_TYPE_ITEM_CLASS_NAME;
    }
    default: {
      slangAssert(false && "Unknown class of type");
    }
  }

  return "";
}

static const char *GetTypeNullValue(const RSExportType *ET) {
  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive: {
      const RSExportPrimitiveType *EPT =
          static_cast<const RSExportPrimitiveType*>(ET);
      if (EPT->isRSObjectType())
        return "null";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeBoolean)
        return "false";
      else
        return "0";
      break;
    }
    case RSExportType::ExportClassPointer:
    case RSExportType::ExportClassVector:
    case RSExportType::ExportClassMatrix:
    case RSExportType::ExportClassConstantArray:
    case RSExportType::ExportClassRecord: {
      return "null";
      break;
    }
    default: {
      slangAssert(false && "Unknown class of type");
    }
  }
  return "";
}

static const char *GetBuiltinElementConstruct(const RSExportType *ET) {
  if (ET->getClass() == RSExportType::ExportClassPrimitive) {
    const RSExportPrimitiveType *EPT =
        static_cast<const RSExportPrimitiveType*>(ET);
    if (EPT->getKind() == RSExportPrimitiveType::DataKindUser) {
      static const char *PrimitiveBuiltinElementConstructMap[] = {
        NULL,               // RSExportPrimitiveType::DataTypeFloat16
        "Element.F32",      // RSExportPrimitiveType::DataTypeFloat32
        "Element.F64",      // RSExportPrimitiveType::DataTypeFloat64
        "Element.I8",       // RSExportPrimitiveType::DataTypeSigned8
        "Element.I16",      // RSExportPrimitiveType::DataTypeSigned16
        "Element.I32",      // RSExportPrimitiveType::DataTypeSigned32
        "Element.I64",      // RSExportPrimitiveType::DataTypeSigned64
        "Element.U8",       // RSExportPrimitiveType::DataTypeUnsigned8
        "Element.U16",      // RSExportPrimitiveType::DataTypeUnsigned16
        "Element.U32",      // RSExportPrimitiveType::DataTypeUnsigned32
        "Element.U64",      // RSExportPrimitiveType::DataTypeUnsigned64
        "Element.BOOLEAN",  // RSExportPrimitiveType::DataTypeBoolean

        NULL,   // RSExportPrimitiveType::DataTypeUnsigned565
        NULL,   // RSExportPrimitiveType::DataTypeUnsigned5551
        NULL,   // RSExportPrimitiveType::DataTypeUnsigned4444

        NULL,   // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix2x2
        NULL,   // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix3x3
        NULL,   // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix4x4

        "Element.ELEMENT",      // RSExportPrimitiveType::DataTypeRSElement
        "Element.TYPE",         // RSExportPrimitiveType::DataTypeRSType
        "Element.ALLOCATION",   // RSExportPrimitiveType::DataTypeRSAllocation
        "Element.SAMPLER",      // RSExportPrimitiveType::DataTypeRSSampler
        "Element.SCRIPT",       // RSExportPrimitiveType::DataTypeRSScript
        "Element.MESH",         // RSExportPrimitiveType::DataTypeRSMesh
        "Element.PROGRAM_FRAGMENT",
          // RSExportPrimitiveType::DataTypeRSProgramFragment
        "Element.PROGRAM_VERTEX",
          // RSExportPrimitiveType::DataTypeRSProgramVertex
        "Element.PROGRAM_RASTER",
          // RSExportPrimitiveType::DataTypeRSProgramRaster
        "Element.PROGRAM_STORE",
          // RSExportPrimitiveType::DataTypeRSProgramStore
        "Element.FONT",
          // RSExportPrimitiveType::DataTypeRSFont
      };
      unsigned TypeId = EPT->getType();

      if (TypeId <
          (sizeof(PrimitiveBuiltinElementConstructMap) / sizeof(const char*)))
        return PrimitiveBuiltinElementConstructMap[ EPT->getType() ];
    } else if (EPT->getKind() == RSExportPrimitiveType::DataKindPixelA) {
      if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
        return "Element.A_8";
    } else if (EPT->getKind() == RSExportPrimitiveType::DataKindPixelRGB) {
      if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned565)
        return "Element.RGB_565";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
        return "Element.RGB_888";
    } else if (EPT->getKind() == RSExportPrimitiveType::DataKindPixelRGBA) {
      if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned5551)
        return "Element.RGBA_5551";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned4444)
        return "Element.RGBA_4444";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
        return "Element.RGBA_8888";
    }
  } else if (ET->getClass() == RSExportType::ExportClassVector) {
    const RSExportVectorType *EVT = static_cast<const RSExportVectorType*>(ET);
    if (EVT->getKind() == RSExportPrimitiveType::DataKindUser) {
      if (EVT->getType() == RSExportPrimitiveType::DataTypeFloat32) {
        if (EVT->getNumElement() == 2)
          return "Element.F32_2";
        else if (EVT->getNumElement() == 3)
          return "Element.F32_3";
        else if (EVT->getNumElement() == 4)
          return "Element.F32_4";
      } else if (EVT->getType() == RSExportPrimitiveType::DataTypeUnsigned8) {
        if (EVT->getNumElement() == 4)
          return "Element.U8_4";
      }
    }
  } else if (ET->getClass() == RSExportType::ExportClassMatrix) {
    const RSExportMatrixType *EMT = static_cast<const RSExportMatrixType *>(ET);
    switch (EMT->getDim()) {
      case 2: return "Element.MATRIX_2X2";
      case 3: return "Element.MATRIX_3X3";
      case 4: return "Element.MATRIX_4X4";
      default: slangAssert(false && "Unsupported dimension of matrix");
    }
  } else if (ET->getClass() == RSExportType::ExportClassPointer) {
    // Treat pointer type variable as unsigned int
    // TODO(zonr): this is target dependent
    return "Element.USER_I32";
  }

  return NULL;
}

static const char *GetElementDataKindName(RSExportPrimitiveType::DataKind DK) {
  static const char *ElementDataKindNameMap[] = {
    "Element.DataKind.USER",        // RSExportPrimitiveType::DataKindUser
    "Element.DataKind.PIXEL_L",     // RSExportPrimitiveType::DataKindPixelL
    "Element.DataKind.PIXEL_A",     // RSExportPrimitiveType::DataKindPixelA
    "Element.DataKind.PIXEL_LA",    // RSExportPrimitiveType::DataKindPixelLA
    "Element.DataKind.PIXEL_RGB",   // RSExportPrimitiveType::DataKindPixelRGB
    "Element.DataKind.PIXEL_RGBA",  // RSExportPrimitiveType::DataKindPixelRGBA
  };

  if (static_cast<unsigned>(DK) <
      (sizeof(ElementDataKindNameMap) / sizeof(const char*)))
    return ElementDataKindNameMap[ DK ];
  else
    return NULL;
}

static const char *GetElementDataTypeName(RSExportPrimitiveType::DataType DT) {
  static const char *ElementDataTypeNameMap[] = {
    NULL,                            // RSExportPrimitiveType::DataTypeFloat16
    "Element.DataType.FLOAT_32",     // RSExportPrimitiveType::DataTypeFloat32
    "Element.DataType.FLOAT_64",     // RSExportPrimitiveType::DataTypeFloat64
    "Element.DataType.SIGNED_8",     // RSExportPrimitiveType::DataTypeSigned8
    "Element.DataType.SIGNED_16",    // RSExportPrimitiveType::DataTypeSigned16
    "Element.DataType.SIGNED_32",    // RSExportPrimitiveType::DataTypeSigned32
    "Element.DataType.SIGNED_64",    // RSExportPrimitiveType::DataTypeSigned64
    "Element.DataType.UNSIGNED_8",   // RSExportPrimitiveType::DataTypeUnsigned8
    "Element.DataType.UNSIGNED_16",
      // RSExportPrimitiveType::DataTypeUnsigned16
    "Element.DataType.UNSIGNED_32",
      // RSExportPrimitiveType::DataTypeUnsigned32
    "Element.DataType.UNSIGNED_64",
      // RSExportPrimitiveType::DataTypeUnsigned64
    "Element.DataType.BOOLEAN",
      // RSExportPrimitiveType::DataTypeBoolean

    // RSExportPrimitiveType::DataTypeUnsigned565
    "Element.DataType.UNSIGNED_5_6_5",
    // RSExportPrimitiveType::DataTypeUnsigned5551
    "Element.DataType.UNSIGNED_5_5_5_1",
    // RSExportPrimitiveType::DataTypeUnsigned4444
    "Element.DataType.UNSIGNED_4_4_4_4",

    // DataTypeRSMatrix* must have been resolved in GetBuiltinElementConstruct()
    NULL,  // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix2x2
    NULL,  // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix3x3
    NULL,  // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix4x4

    "Element.DataType.RS_ELEMENT",  // RSExportPrimitiveType::DataTypeRSElement
    "Element.DataType.RS_TYPE",     // RSExportPrimitiveType::DataTypeRSType
      // RSExportPrimitiveType::DataTypeRSAllocation
    "Element.DataType.RS_ALLOCATION",
      // RSExportPrimitiveType::DataTypeRSSampler
    "Element.DataType.RS_SAMPLER",
      // RSExportPrimitiveType::DataTypeRSScript
    "Element.DataType.RS_SCRIPT",
      // RSExportPrimitiveType::DataTypeRSMesh
    "Element.DataType.RS_MESH",
      // RSExportPrimitiveType::DataTypeRSProgramFragment
    "Element.DataType.RS_PROGRAM_FRAGMENT",
      // RSExportPrimitiveType::DataTypeRSProgramVertex
    "Element.DataType.RS_PROGRAM_VERTEX",
      // RSExportPrimitiveType::DataTypeRSProgramRaster
    "Element.DataType.RS_PROGRAM_RASTER",
      // RSExportPrimitiveType::DataTypeRSProgramStore
    "Element.DataType.RS_PROGRAM_STORE",
      // RSExportPrimitiveType::DataTypeRSFont
    "Element.DataType.RS_FONT",
  };

  if (static_cast<unsigned>(DT) <
      (sizeof(ElementDataTypeNameMap) / sizeof(const char*)))
    return ElementDataTypeNameMap[ DT ];
  else
    return NULL;
}

static const char *GetElementJavaTypeName(RSExportPrimitiveType::DataType DT) {
  static const char *ElementJavaTypeNameMap[] = {
    NULL,                 // RSExportPrimitiveType::DataTypeFloat16
    "F32",                // RSExportPrimitiveType::DataTypeFloat32
    "F64",                // RSExportPrimitiveType::DataTypeFloat64
    "I8",                 // RSExportPrimitiveType::DataTypeSigned8
    "I16",                // RSExportPrimitiveType::DataTypeSigned16
    "I32",                // RSExportPrimitiveType::DataTypeSigned32
    "I64",                // RSExportPrimitiveType::DataTypeSigned64
    "U8",                 // RSExportPrimitiveType::DataTypeUnsigned8
    "U16",                // RSExportPrimitiveType::DataTypeUnsigned16
    "U32",                // RSExportPrimitiveType::DataTypeUnsigned32
    "U64",                // RSExportPrimitiveType::DataTypeUnsigned64
    "BOOLEAN",            // RSExportPrimitiveType::DataTypeBoolean

    "RGB_565",            // RSExportPrimitiveType::DataTypeUnsigned565
    "RGBA_5551",          // RSExportPrimitiveType::DataTypeUnsigned5551
    "RGBA_4444",          // RSExportPrimitiveType::DataTypeUnsigned4444

    // DataTypeRSMatrix* must have been resolved in GetBuiltinElementConstruct()
    NULL,                 // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix2x2
    NULL,                 // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix3x3
    NULL,                 // (Dummy) RSExportPrimitiveType::DataTypeRSMatrix4x4

    "ELEMENT",            // RSExportPrimitiveType::DataTypeRSElement
    "TYPE",               // RSExportPrimitiveType::DataTypeRSType
    "ALLOCATION",         // RSExportPrimitiveType::DataTypeRSAllocation
    "SAMPLER",            // RSExportPrimitiveType::DataTypeRSSampler
    "SCRIPT",             // RSExportPrimitiveType::DataTypeRSScript
    "MESH",               // RSExportPrimitiveType::DataTypeRSMesh
    "PROGRAM_FRAGMENT",   // RSExportPrimitiveType::DataTypeRSProgramFragment
    "PROGRAM_VERTEX",     // RSExportPrimitiveType::DataTypeRSProgramVertex
    "PROGRAM_RASTER",     // RSExportPrimitiveType::DataTypeRSProgramRaster
    "PROGRAM_STORE",      // RSExportPrimitiveType::DataTypeRSProgramStore
    "FONT",               // RSExportPrimitiveType::DataTypeRSFont
  };

  if (static_cast<unsigned>(DT) <
      (sizeof(ElementJavaTypeNameMap) / sizeof(const char*)))
    return ElementJavaTypeNameMap[DT];
  else
    return NULL;
}


/********************** Methods to generate script class **********************/
bool RSReflection::genScriptClass(Context &C,
                                  const std::string &ClassName,
                                  std::string &ErrorMsg) {
  if (!C.startClass(Context::AM_Public,
                    false,
                    ClassName,
                    RS_SCRIPT_CLASS_SUPER_CLASS_NAME,
                    ErrorMsg))
    return false;

  genScriptClassConstructor(C);

  // Reflect export variable
  for (RSContext::const_export_var_iterator I = mRSContext->export_vars_begin(),
           E = mRSContext->export_vars_end();
       I != E;
       I++)
    genExportVariable(C, *I);

  // Reflect export for each functions (only available on ICS+)
  if (mRSContext->getTargetAPI() >= SLANG_ICS_TARGET_API) {
    for (RSContext::const_export_foreach_iterator
             I = mRSContext->export_foreach_begin(),
             E = mRSContext->export_foreach_end();
         I != E; I++)
      genExportForEach(C, *I);
  }

  // Reflect export function
  for (RSContext::const_export_func_iterator
           I = mRSContext->export_funcs_begin(),
           E = mRSContext->export_funcs_end();
       I != E; I++)
    genExportFunction(C, *I);

  C.endClass();

  return true;
}

void RSReflection::genScriptClassConstructor(Context &C) {
  C.indent() << "// Constructor" << std::endl;
  C.startFunction(Context::AM_Public,
                  false,
                  NULL,
                  C.getClassName(),
                  3,
                  "RenderScript", "rs",
                  "Resources", "resources",
                  "int", "id");
  // Call constructor of super class
  C.indent() << "super(rs, resources, id);" << std::endl;

  // If an exported variable has initial value, reflect it

  for (RSContext::const_export_var_iterator I = mRSContext->export_vars_begin(),
           E = mRSContext->export_vars_end();
       I != E;
       I++) {
    const RSExportVar *EV = *I;
    if (!EV->getInit().isUninit())
      genInitExportVariable(C, EV->getType(), EV->getName(), EV->getInit());
  }

  for (RSContext::const_export_foreach_iterator
           I = mRSContext->export_foreach_begin(),
           E = mRSContext->export_foreach_end();
       I != E;
       I++) {
    const RSExportForEach *EF = *I;

    const RSExportType *IET = EF->getInType();
    if (IET) {
      genTypeInstance(C, IET);
    }
    const RSExportType *OET = EF->getOutType();
    if (OET) {
      genTypeInstance(C, OET);
    }
  }

  C.endFunction();

  for (std::set<std::string>::iterator I = C.mTypesToCheck.begin(),
                                       E = C.mTypesToCheck.end();
       I != E;
       I++) {
    C.indent() << "private Element __" << *I << ";" << std::endl;
  }

  return;
}

void RSReflection::genInitBoolExportVariable(Context &C,
                                             const std::string &VarName,
                                             const clang::APValue &Val) {
  slangAssert(!Val.isUninit() && "Not a valid initializer");

  C.indent() << RS_EXPORT_VAR_PREFIX << VarName << " = ";
  slangAssert((Val.getKind() == clang::APValue::Int) &&
              "Bool type has wrong initial APValue");

  C.out() << ((Val.getInt().getSExtValue() == 0) ? "false" : "true")
          << ";" << std::endl;

  return;
}

void RSReflection::genInitPrimitiveExportVariable(
      Context &C,
      const std::string &VarName,
      const clang::APValue &Val) {
  slangAssert(!Val.isUninit() && "Not a valid initializer");

  C.indent() << RS_EXPORT_VAR_PREFIX << VarName << " = ";
  switch (Val.getKind()) {
    case clang::APValue::Int: {
      llvm::APInt api = Val.getInt();
      C.out() << api.getSExtValue();
      if (api.getBitWidth() > 32) {
        C.out() << "L";
      }
      break;
    }
    case clang::APValue::Float: {
      llvm::APFloat apf = Val.getFloat();
      if (&apf.getSemantics() == &llvm::APFloat::IEEEsingle) {
        C.out() << apf.convertToFloat() << "f";
      } else {
        C.out() << apf.convertToDouble();
      }
      break;
    }

    case clang::APValue::ComplexInt:
    case clang::APValue::ComplexFloat:
    case clang::APValue::LValue:
    case clang::APValue::Vector: {
      slangAssert(false &&
                  "Primitive type cannot have such kind of initializer");
      break;
    }
    default: {
      slangAssert(false && "Unknown kind of initializer");
    }
  }
  C.out() << ";" << std::endl;

  return;
}

void RSReflection::genInitExportVariable(Context &C,
                                         const RSExportType *ET,
                                         const std::string &VarName,
                                         const clang::APValue &Val) {
  slangAssert(!Val.isUninit() && "Not a valid initializer");

  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive: {
      const RSExportPrimitiveType *EPT =
          static_cast<const RSExportPrimitiveType*>(ET);
      if (EPT->getType() == RSExportPrimitiveType::DataTypeBoolean) {
        genInitBoolExportVariable(C, VarName, Val);
      } else {
        genInitPrimitiveExportVariable(C, VarName, Val);
      }
      break;
    }
    case RSExportType::ExportClassPointer: {
      if (!Val.isInt() || Val.getInt().getSExtValue() != 0)
        std::cout << "Initializer which is non-NULL to pointer type variable "
                     "will be ignored" << std::endl;
      break;
    }
    case RSExportType::ExportClassVector: {
      const RSExportVectorType *EVT =
          static_cast<const RSExportVectorType*>(ET);
      switch (Val.getKind()) {
        case clang::APValue::Int:
        case clang::APValue::Float: {
          for (unsigned i = 0; i < EVT->getNumElement(); i++) {
            std::string Name =  VarName + "." + GetVectorAccessor(i);
            genInitPrimitiveExportVariable(C, Name, Val);
          }
          break;
        }
        case clang::APValue::Vector: {
          C.indent() << RS_EXPORT_VAR_PREFIX << VarName << " = new "
                     << GetVectorTypeName(EVT) << "();" << std::endl;

          unsigned NumElements =
              std::min(static_cast<unsigned>(EVT->getNumElement()),
                       Val.getVectorLength());
          for (unsigned i = 0; i < NumElements; i++) {
            const clang::APValue &ElementVal = Val.getVectorElt(i);
            std::string Name = VarName + "." + GetVectorAccessor(i);
            genInitPrimitiveExportVariable(C, Name, ElementVal);
          }
          break;
        }
        case clang::APValue::Uninitialized:
        case clang::APValue::ComplexInt:
        case clang::APValue::ComplexFloat:
        case clang::APValue::LValue: {
          slangAssert(false && "Unexpected type of value of initializer.");
        }
      }
      break;
    }
    // TODO(zonr): Resolving initializer of a record (and matrix) type variable
    // is complex. It cannot obtain by just simply evaluating the initializer
    // expression.
    case RSExportType::ExportClassMatrix:
    case RSExportType::ExportClassConstantArray:
    case RSExportType::ExportClassRecord: {
#if 0
      unsigned InitIndex = 0;
      const RSExportRecordType *ERT =
          static_cast<const RSExportRecordType*>(ET);

      slangAssert((Val.getKind() == clang::APValue::Vector) &&
          "Unexpected type of initializer for record type variable");

      C.indent() << RS_EXPORT_VAR_PREFIX << VarName
                 << " = new "RS_TYPE_CLASS_NAME_PREFIX << ERT->getName()
                 <<  "."RS_TYPE_ITEM_CLASS_NAME"();" << std::endl;

      for (RSExportRecordType::const_field_iterator I = ERT->fields_begin(),
               E = ERT->fields_end();
           I != E;
           I++) {
        const RSExportRecordType::Field *F = *I;
        std::string FieldName = VarName + "." + F->getName();

        if (InitIndex > Val.getVectorLength())
          break;

        genInitPrimitiveExportVariable(C,
                                       FieldName,
                                       Val.getVectorElt(InitIndex++));
      }
#endif
      slangAssert(false && "Unsupported initializer for record/matrix/constant "
                           "array type variable currently");
      break;
    }
    default: {
      slangAssert(false && "Unknown class of type");
    }
  }
  return;
}

void RSReflection::genExportVariable(Context &C, const RSExportVar *EV) {
  const RSExportType *ET = EV->getType();

  C.indent() << "private final static int "RS_EXPORT_VAR_INDEX_PREFIX
             << EV->getName() << " = " << C.getNextExportVarSlot() << ";"
             << std::endl;

  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive: {
      genPrimitiveTypeExportVariable(C, EV);
      break;
    }
    case RSExportType::ExportClassPointer: {
      genPointerTypeExportVariable(C, EV);
      break;
    }
    case RSExportType::ExportClassVector: {
      genVectorTypeExportVariable(C, EV);
      break;
    }
    case RSExportType::ExportClassMatrix: {
      genMatrixTypeExportVariable(C, EV);
      break;
    }
    case RSExportType::ExportClassConstantArray: {
      genConstantArrayTypeExportVariable(C, EV);
      break;
    }
    case RSExportType::ExportClassRecord: {
      genRecordTypeExportVariable(C, EV);
      break;
    }
    default: {
      slangAssert(false && "Unknown class of type");
    }
  }

  return;
}

void RSReflection::genExportFunction(Context &C, const RSExportFunc *EF) {
  C.indent() << "private final static int "RS_EXPORT_FUNC_INDEX_PREFIX
             << EF->getName() << " = " << C.getNextExportFuncSlot() << ";"
             << std::endl;

  // invoke_*()
  Context::ArgTy Args;

  if (EF->hasParam()) {
    for (RSExportFunc::const_param_iterator I = EF->params_begin(),
             E = EF->params_end();
         I != E;
         I++) {
      Args.push_back(std::make_pair(GetTypeName((*I)->getType()),
                                    (*I)->getName()));
    }
  }

  C.startFunction(Context::AM_Public,
                  false,
                  "void",
                  "invoke_" + EF->getName(/*Mangle=*/ false),
                      // We are using un-mangled name since Java
                      // supports method overloading.
                  Args);

  if (!EF->hasParam()) {
    C.indent() << "invoke("RS_EXPORT_FUNC_INDEX_PREFIX << EF->getName() << ");"
               << std::endl;
  } else {
    const RSExportRecordType *ERT = EF->getParamPacketType();
    std::string FieldPackerName = EF->getName() + "_fp";

    if (genCreateFieldPacker(C, ERT, FieldPackerName.c_str()))
      genPackVarOfType(C, ERT, NULL, FieldPackerName.c_str());

    C.indent() << "invoke("RS_EXPORT_FUNC_INDEX_PREFIX << EF->getName() << ", "
               << FieldPackerName << ");" << std::endl;
  }

  C.endFunction();
  return;
}

void RSReflection::genExportForEach(Context &C, const RSExportForEach *EF) {
  C.indent() << "private final static int "RS_EXPORT_FOREACH_INDEX_PREFIX
             << EF->getName() << " = " << C.getNextExportForEachSlot() << ";"
             << std::endl;

  // forEach_*()
  Context::ArgTy Args;

  slangAssert(EF->getNumParameters() > 0);

  if (EF->hasIn())
    Args.push_back(std::make_pair("Allocation", "ain"));
  if (EF->hasOut())
    Args.push_back(std::make_pair("Allocation", "aout"));

  const RSExportRecordType *ERT = EF->getParamPacketType();
  if (ERT) {
    for (RSExportForEach::const_param_iterator I = EF->params_begin(),
             E = EF->params_end();
         I != E;
         I++) {
      Args.push_back(std::make_pair(GetTypeName((*I)->getType()),
                                    (*I)->getName()));
    }
  }

  C.startFunction(Context::AM_Public,
                  false,
                  "void",
                  "forEach_" + EF->getName(),
                  Args);

  const RSExportType *IET = EF->getInType();
  if (IET) {
    genTypeCheck(C, IET, "ain");
  }

  const RSExportType *OET = EF->getOutType();
  if (OET) {
    genTypeCheck(C, OET, "aout");
  }

  if (EF->hasIn() && EF->hasOut()) {
    C.indent() << "// Verify dimensions" << std::endl;
    C.indent() << "Type tIn = ain.getType();" << std::endl;
    C.indent() << "Type tOut = aout.getType();" << std::endl;
    C.indent() << "if ((tIn.getCount() != tOut.getCount()) ||" << std::endl;
    C.indent() << "    (tIn.getX() != tOut.getX()) ||" << std::endl;
    C.indent() << "    (tIn.getY() != tOut.getY()) ||" << std::endl;
    C.indent() << "    (tIn.getZ() != tOut.getZ()) ||" << std::endl;
    C.indent() << "    (tIn.hasFaces() != tOut.hasFaces()) ||" << std::endl;
    C.indent() << "    (tIn.hasMipmaps() != tOut.hasMipmaps())) {" << std::endl;
    C.indent() << "    throw new RSRuntimeException(\"Dimension mismatch "
               << "between input and output parameters!\");";
    C.out()    << std::endl;
    C.indent() << "}" << std::endl;
  }

  std::string FieldPackerName = EF->getName() + "_fp";
  if (ERT) {
    if (genCreateFieldPacker(C, ERT, FieldPackerName.c_str())) {
      genPackVarOfType(C, ERT, NULL, FieldPackerName.c_str());
    }
  }
  C.indent() << "forEach("RS_EXPORT_FOREACH_INDEX_PREFIX << EF->getName();

  if (EF->hasIn())
    C.out() << ", ain";
  else
    C.out() << ", null";

  if (EF->hasOut())
    C.out() << ", aout";
  else
    C.out() << ", null";

  if (EF->hasUsrData())
    C.out() << ", " << FieldPackerName;
  else
    C.out() << ", null";

  C.out() << ");" << std::endl;

  C.endFunction();
  return;
}

void RSReflection::genTypeInstance(Context &C,
                                   const RSExportType *ET) {
  if (ET->getClass() == RSExportType::ExportClassPointer) {
    const RSExportPointerType *EPT =
        static_cast<const RSExportPointerType*>(ET);
    ET = EPT->getPointeeType();
    switch (ET->getClass()) {
      case RSExportType::ExportClassPrimitive: {
        const RSExportPrimitiveType *EPT =
            static_cast<const RSExportPrimitiveType*>(ET);
        slangAssert(EPT);

        switch (EPT->getKind()) {
          case RSExportPrimitiveType::DataKindPixelL:
          case RSExportPrimitiveType::DataKindPixelA:
          case RSExportPrimitiveType::DataKindPixelLA:
          case RSExportPrimitiveType::DataKindPixelRGB:
          case RSExportPrimitiveType::DataKindPixelRGBA: {
            break;
          }

          case RSExportPrimitiveType::DataKindUser:
          default: {
            std::string TypeName = GetElementJavaTypeName(EPT->getType());
            if (C.mTypesToCheck.find(TypeName) == C.mTypesToCheck.end()) {
              C.indent() << "__" << TypeName << " = Element." << TypeName
                         << "(rs);" << std::endl;
              C.mTypesToCheck.insert(TypeName);
            }
            break;
          }
        }
        break;
      }

      case RSExportType::ExportClassVector: {
        const RSExportVectorType *EVT =
            static_cast<const RSExportVectorType*>(ET);
        slangAssert(EVT);

        const char *TypeName = GetVectorElementName(EVT);
        if (C.mTypesToCheck.find(TypeName) == C.mTypesToCheck.end()) {
          C.indent() << "__" << TypeName << " = Element." << TypeName
                     << "(rs);" << std::endl;
          C.mTypesToCheck.insert(TypeName);
        }
        break;
      }

      case RSExportType::ExportClassRecord: {
        const RSExportRecordType *ERT =
            static_cast<const RSExportRecordType*>(ET);
        slangAssert(ERT);

        std::string ClassName = RS_TYPE_CLASS_NAME_PREFIX + ERT->getName();
        if (C.mTypesToCheck.find(ClassName) == C.mTypesToCheck.end()) {
          C.indent() << "__" << ClassName << " = " << ClassName <<
                        ".createElement(rs);" << std::endl;
          C.mTypesToCheck.insert(ClassName);
        }
        break;
      }

      default:
        break;
    }
  }
}

void RSReflection::genTypeCheck(Context &C,
                                const RSExportType *ET,
                                const char *VarName) {
  C.indent() << "// check " << VarName << std::endl;

  if (ET->getClass() == RSExportType::ExportClassPointer) {
    const RSExportPointerType *EPT =
        static_cast<const RSExportPointerType*>(ET);
    ET = EPT->getPointeeType();
  }

  std::string TypeName;

  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive: {
      const RSExportPrimitiveType *EPT =
          static_cast<const RSExportPrimitiveType*>(ET);
      slangAssert(EPT);

      if (EPT->getKind() == RSExportPrimitiveType::DataKindUser) {
        TypeName = GetElementJavaTypeName(EPT->getType());
      }
      break;
    }

    case RSExportType::ExportClassVector: {
      const RSExportVectorType *EVT =
          static_cast<const RSExportVectorType*>(ET);
      slangAssert(EVT);
      TypeName = GetVectorElementName(EVT);
      break;
    }

    case RSExportType::ExportClassRecord: {
      const RSExportRecordType *ERT =
          static_cast<const RSExportRecordType*>(ET);
      slangAssert(ERT);
      TypeName = RS_TYPE_CLASS_NAME_PREFIX + ERT->getName();
      break;
    }

    default:
      break;
  }

  if (!TypeName.empty()) {
    C.indent() << "if (!" << VarName
               << ".getType().getElement().isCompatible(__"
               << TypeName << ")) {" << std::endl;
    C.indent() << "    throw new RSRuntimeException(\"Type mismatch with "
               << TypeName << "!\");" << std::endl;
    C.indent() << "}" << std::endl;
  }

  return;
}


void RSReflection::genPrimitiveTypeExportVariable(
    Context &C,
    const RSExportVar *EV) {
  slangAssert((EV->getType()->getClass() == RSExportType::ExportClassPrimitive)
              && "Variable should be type of primitive here");

  const RSExportPrimitiveType *EPT =
      static_cast<const RSExportPrimitiveType*>(EV->getType());
  const char *TypeName = GetPrimitiveTypeName(EPT);

  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  // set_*()
  if (!EV->isConst()) {
    C.startFunction(Context::AM_Public,
                    false,
                    "void",
                    "set_" + EV->getName(),
                    1,
                    TypeName, "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;

    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName()
               << ", v);" << std::endl;

    C.endFunction();
  }

  genGetExportVariable(C, TypeName, EV->getName());

  return;
}

void RSReflection::genPointerTypeExportVariable(Context &C,
                                                const RSExportVar *EV) {
  const RSExportType *ET = EV->getType();
  const RSExportType *PointeeType;
  std::string TypeName;

  slangAssert((ET->getClass() == RSExportType::ExportClassPointer) &&
              "Variable should be type of pointer here");

  PointeeType = static_cast<const RSExportPointerType*>(ET)->getPointeeType();
  TypeName = GetTypeName(ET);

  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  // bind_*()
  C.startFunction(Context::AM_Public,
                  false,
                  "void",
                  "bind_" + EV->getName(),
                  1,
                  TypeName.c_str(), "v");

  C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;
  C.indent() << "if (v == null) bindAllocation(null, "RS_EXPORT_VAR_INDEX_PREFIX
             << EV->getName() << ");" << std::endl;

  if (PointeeType->getClass() == RSExportType::ExportClassRecord)
    C.indent() << "else bindAllocation(v.getAllocation(), "
        RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ");"
               << std::endl;
  else
    C.indent() << "else bindAllocation(v, "RS_EXPORT_VAR_INDEX_PREFIX
               << EV->getName() << ");" << std::endl;

  C.endFunction();

  genGetExportVariable(C, TypeName, EV->getName());

  return;
}

void RSReflection::genVectorTypeExportVariable(Context &C,
                                               const RSExportVar *EV) {
  slangAssert((EV->getType()->getClass() == RSExportType::ExportClassVector) &&
              "Variable should be type of vector here");

  const RSExportVectorType *EVT =
      static_cast<const RSExportVectorType*>(EV->getType());
  const char *TypeName = GetVectorTypeName(EVT);
  const char *FieldPackerName = "fp";

  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  // set_*()
  if (!EV->isConst()) {
    C.startFunction(Context::AM_Public,
                    false,
                    "void",
                    "set_" + EV->getName(),
                    1,
                    TypeName, "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;

    if (genCreateFieldPacker(C, EVT, FieldPackerName))
      genPackVarOfType(C, EVT, "v", FieldPackerName);
    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", "
               << FieldPackerName << ");" << std::endl;

    C.endFunction();
  }

  genGetExportVariable(C, TypeName, EV->getName());
  return;
}

void RSReflection::genMatrixTypeExportVariable(Context &C,
                                               const RSExportVar *EV) {
  slangAssert((EV->getType()->getClass() == RSExportType::ExportClassMatrix) &&
              "Variable should be type of matrix here");

  const RSExportMatrixType *EMT =
      static_cast<const RSExportMatrixType*>(EV->getType());
  const char *TypeName = GetMatrixTypeName(EMT);
  const char *FieldPackerName = "fp";

  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  // set_*()
  if (!EV->isConst()) {
    C.startFunction(Context::AM_Public,
                    false,
                    "void",
                    "set_" + EV->getName(),
                    1,
                    TypeName, "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;

    if (genCreateFieldPacker(C, EMT, FieldPackerName))
      genPackVarOfType(C, EMT, "v", FieldPackerName);
    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", "
               << FieldPackerName << ");" << std::endl;

    C.endFunction();
  }

  genGetExportVariable(C, TypeName, EV->getName());
  return;
}

void RSReflection::genConstantArrayTypeExportVariable(
    Context &C,
    const RSExportVar *EV) {
  slangAssert((EV->getType()->getClass() ==
               RSExportType::ExportClassConstantArray) &&
              "Variable should be type of constant array here");

  const RSExportConstantArrayType *ECAT =
      static_cast<const RSExportConstantArrayType*>(EV->getType());
  std::string TypeName = GetTypeName(ECAT);
  const char *FieldPackerName = "fp";

  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  // set_*()
  if (!EV->isConst()) {
    C.startFunction(Context::AM_Public,
                    false,
                    "void",
                    "set_" + EV->getName(),
                    1,
                    TypeName.c_str(), "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;

    if (genCreateFieldPacker(C, ECAT, FieldPackerName))
      genPackVarOfType(C, ECAT, "v", FieldPackerName);
    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", "
               << FieldPackerName << ");" << std::endl;

    C.endFunction();
  }

  genGetExportVariable(C, TypeName, EV->getName());
  return;
}

void RSReflection::genRecordTypeExportVariable(Context &C,
                                               const RSExportVar *EV) {
  slangAssert((EV->getType()->getClass() == RSExportType::ExportClassRecord) &&
              "Variable should be type of struct here");

  const RSExportRecordType *ERT =
      static_cast<const RSExportRecordType*>(EV->getType());
  std::string TypeName =
      RS_TYPE_CLASS_NAME_PREFIX + ERT->getName() + "."RS_TYPE_ITEM_CLASS_NAME;
  const char *FieldPackerName = "fp";

  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  // set_*()
  if (!EV->isConst()) {
    C.startFunction(Context::AM_Public,
                    false,
                    "void",
                    "set_" + EV->getName(),
                    1,
                    TypeName.c_str(), "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;

    if (genCreateFieldPacker(C, ERT, FieldPackerName))
      genPackVarOfType(C, ERT, "v", FieldPackerName);
    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName()
               << ", " << FieldPackerName << ");" << std::endl;

    C.endFunction();
  }

  genGetExportVariable(C, TypeName.c_str(), EV->getName());
  return;
}

void RSReflection::genGetExportVariable(Context &C,
                                        const std::string &TypeName,
                                        const std::string &VarName) {
  C.startFunction(Context::AM_Public,
                  false,
                  TypeName.c_str(),
                  "get_" + VarName,
                  0);

  C.indent() << "return "RS_EXPORT_VAR_PREFIX << VarName << ";" << std::endl;

  C.endFunction();
  return;
}

/******************* Methods to generate script class /end *******************/

bool RSReflection::genCreateFieldPacker(Context &C,
                                        const RSExportType *ET,
                                        const char *FieldPackerName) {
  size_t AllocSize = RSExportType::GetTypeAllocSize(ET);
  if (AllocSize > 0)
    C.indent() << "FieldPacker " << FieldPackerName << " = new FieldPacker("
               << AllocSize << ");" << std::endl;
  else
    return false;
  return true;
}

void RSReflection::genPackVarOfType(Context &C,
                                    const RSExportType *ET,
                                    const char *VarName,
                                    const char *FieldPackerName) {
  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive:
    case RSExportType::ExportClassVector: {
      C.indent() << FieldPackerName << "."
                 << GetPackerAPIName(
                     static_cast<const RSExportPrimitiveType*>(ET))
                 << "(" << VarName << ");" << std::endl;
      break;
    }
    case RSExportType::ExportClassPointer: {
      // Must reflect as type Allocation in Java
      const RSExportType *PointeeType =
          static_cast<const RSExportPointerType*>(ET)->getPointeeType();

      if (PointeeType->getClass() != RSExportType::ExportClassRecord)
        C.indent() << FieldPackerName << ".addI32(" << VarName
                   << ".getPtr());" << std::endl;
      else
        C.indent() << FieldPackerName << ".addI32(" << VarName
                   << ".getAllocation().getPtr());" << std::endl;
      break;
    }
    case RSExportType::ExportClassMatrix: {
      C.indent() << FieldPackerName << ".addMatrix(" << VarName << ");"
                 << std::endl;
      break;
    }
    case RSExportType::ExportClassConstantArray: {
      const RSExportConstantArrayType *ECAT =
          static_cast<const RSExportConstantArrayType *>(ET);

      // TODO(zonr): more elegant way. Currently, we obtain the unique index
      //             variable (this method involves recursive call which means
      //             we may have more than one level loop, therefore we can't
      //             always use the same index variable name here) name given
      //             in the for-loop from counting the '.' in @VarName.
      unsigned Level = 0;
      size_t LastDotPos = 0;
      std::string ElementVarName(VarName);

      while (LastDotPos != std::string::npos) {
        LastDotPos = ElementVarName.find_first_of('.', LastDotPos + 1);
        Level++;
      }
      std::string IndexVarName("ct");
      IndexVarName.append(llvm::utostr_32(Level));

      C.indent() << "for (int " << IndexVarName << " = 0; " <<
                          IndexVarName << " < " << ECAT->getSize() << "; " <<
                          IndexVarName << "++)";
      C.startBlock();

      ElementVarName.append("[" + IndexVarName + "]");
      genPackVarOfType(C, ECAT->getElementType(), ElementVarName.c_str(),
                       FieldPackerName);

      C.endBlock();
      break;
    }
    case RSExportType::ExportClassRecord: {
      const RSExportRecordType *ERT =
          static_cast<const RSExportRecordType*>(ET);
      // Relative pos from now on in field packer
      unsigned Pos = 0;

      for (RSExportRecordType::const_field_iterator I = ERT->fields_begin(),
               E = ERT->fields_end();
           I != E;
           I++) {
        const RSExportRecordType::Field *F = *I;
        std::string FieldName;
        size_t FieldOffset = F->getOffsetInParent();
        size_t FieldStoreSize = RSExportType::GetTypeStoreSize(F->getType());
        size_t FieldAllocSize = RSExportType::GetTypeAllocSize(F->getType());

        if (VarName != NULL)
          FieldName = VarName + ("." + F->getName());
        else
          FieldName = F->getName();

        if (FieldOffset > Pos)
          C.indent() << FieldPackerName << ".skip("
                     << (FieldOffset - Pos) << ");" << std::endl;

        genPackVarOfType(C, F->getType(), FieldName.c_str(), FieldPackerName);

        // There is padding in the field type
        if (FieldAllocSize > FieldStoreSize)
            C.indent() << FieldPackerName << ".skip("
                       << (FieldAllocSize - FieldStoreSize)
                       << ");" << std::endl;

          Pos = FieldOffset + FieldAllocSize;
      }

      // There maybe some padding after the struct
      if (RSExportType::GetTypeAllocSize(ERT) > Pos)
        C.indent() << FieldPackerName << ".skip("
                   << RSExportType::GetTypeAllocSize(ERT) - Pos << ");"
                   << std::endl;
      break;
    }
    default: {
      slangAssert(false && "Unknown class of type");
    }
  }

  return;
}

void RSReflection::genAllocateVarOfType(Context &C,
                                        const RSExportType *T,
                                        const std::string &VarName) {
  switch (T->getClass()) {
    case RSExportType::ExportClassPrimitive: {
      // Primitive type like int in Java has its own storage once it's declared.
      //
      // FIXME: Should we allocate storage for RS object?
      // if (static_cast<const RSExportPrimitiveType *>(T)->isRSObjectType())
      //  C.indent() << VarName << " = new " << GetTypeName(T) << "();"
      //             << std::endl;
      break;
    }
    case RSExportType::ExportClassPointer: {
      // Pointer type is an instance of Allocation or a TypeClass whose value is
      // expected to be assigned by programmer later in Java program. Therefore
      // we don't reflect things like [VarName] = new Allocation();
      C.indent() << VarName << " = null;" << std::endl;
      break;
    }
    case RSExportType::ExportClassConstantArray: {
      const RSExportConstantArrayType *ECAT =
          static_cast<const RSExportConstantArrayType *>(T);
      const RSExportType *ElementType = ECAT->getElementType();

      C.indent() << VarName << " = new " << GetTypeName(ElementType)
                 << "[" << ECAT->getSize() << "];" << std::endl;

      // Primitive type element doesn't need allocation code.
      if (ElementType->getClass() != RSExportType::ExportClassPrimitive) {
        C.indent() << "for (int $ct = 0; $ct < " << ECAT->getSize() << "; "
                            "$ct++)";
        C.startBlock();

        std::string ElementVarName(VarName);
        ElementVarName.append("[$ct]");
        genAllocateVarOfType(C, ElementType, ElementVarName);

        C.endBlock();
      }
      break;
    }
    case RSExportType::ExportClassVector:
    case RSExportType::ExportClassMatrix:
    case RSExportType::ExportClassRecord: {
      C.indent() << VarName << " = new " << GetTypeName(T) << "();"
                 << std::endl;
      break;
    }
  }
  return;
}

void RSReflection::genNewItemBufferIfNull(Context &C,
                                          const char *Index) {
  C.indent() << "if (" RS_TYPE_ITEM_BUFFER_NAME " == null) "
                  RS_TYPE_ITEM_BUFFER_NAME " = "
                    "new " RS_TYPE_ITEM_CLASS_NAME
                      "[getType().getX() /* count */];"
             << std::endl;
  if (Index != NULL)
    C.indent() << "if ("RS_TYPE_ITEM_BUFFER_NAME"[" << Index << "] == null) "
                    RS_TYPE_ITEM_BUFFER_NAME"[" << Index << "] = "
                      "new "RS_TYPE_ITEM_CLASS_NAME"();" << std::endl;
  return;
}

void RSReflection::genNewItemBufferPackerIfNull(Context &C) {
  C.indent() << "if (" RS_TYPE_ITEM_BUFFER_PACKER_NAME " == null) "
                  RS_TYPE_ITEM_BUFFER_PACKER_NAME " = "
                    "new FieldPacker(" RS_TYPE_ITEM_CLASS_NAME
                      ".sizeof * getType().getX()/* count */"
                        ");" << std::endl;
  return;
}

/********************** Methods to generate type class  **********************/
bool RSReflection::genTypeClass(Context &C,
                                const RSExportRecordType *ERT,
                                std::string &ErrorMsg) {
  std::string ClassName = RS_TYPE_CLASS_NAME_PREFIX + ERT->getName();

  if (!C.startClass(Context::AM_Public,
                    false,
                    ClassName,
                    RS_TYPE_CLASS_SUPER_CLASS_NAME,
                    ErrorMsg))
    return false;

  mGeneratedFileNames->push_back(ClassName);

  genTypeItemClass(C, ERT);

  // Declare item buffer and item buffer packer
  C.indent() << "private "RS_TYPE_ITEM_CLASS_NAME" "RS_TYPE_ITEM_BUFFER_NAME"[]"
      ";" << std::endl;
  C.indent() << "private FieldPacker "RS_TYPE_ITEM_BUFFER_PACKER_NAME";"
             << std::endl;

  genTypeClassConstructor(C, ERT);
  genTypeClassCopyToArrayLocal(C, ERT);
  genTypeClassCopyToArray(C, ERT);
  genTypeClassItemSetter(C, ERT);
  genTypeClassItemGetter(C, ERT);
  genTypeClassComponentSetter(C, ERT);
  genTypeClassComponentGetter(C, ERT);
  genTypeClassCopyAll(C, ERT);
  genTypeClassResize(C);

  C.endClass();

  C.resetFieldIndex();
  C.clearFieldIndexMap();

  return true;
}

void RSReflection::genTypeItemClass(Context &C,
                                    const RSExportRecordType *ERT) {
  C.indent() << "static public class "RS_TYPE_ITEM_CLASS_NAME;
  C.startBlock();

  C.indent() << "public static final int sizeof = "
             << RSExportType::GetTypeAllocSize(ERT) << ";" << std::endl;

  // Member elements
  C.out() << std::endl;
  for (RSExportRecordType::const_field_iterator FI = ERT->fields_begin(),
           FE = ERT->fields_end();
       FI != FE;
       FI++) {
    C.indent() << GetTypeName((*FI)->getType()) << " " << (*FI)->getName()
               << ";" << std::endl;
  }

  // Constructor
  C.out() << std::endl;
  C.indent() << RS_TYPE_ITEM_CLASS_NAME"()";
  C.startBlock();

  for (RSExportRecordType::const_field_iterator FI = ERT->fields_begin(),
           FE = ERT->fields_end();
       FI != FE;
       FI++) {
    const RSExportRecordType::Field *F = *FI;
    genAllocateVarOfType(C, F->getType(), F->getName());
  }

  // end Constructor
  C.endBlock();

  // end Item class
  C.endBlock();

  return;
}

void RSReflection::genTypeClassConstructor(Context &C,
                                           const RSExportRecordType *ERT) {
  const char *RenderScriptVar = "rs";

  C.startFunction(Context::AM_Public,
                  true,
                  "Element",
                  "createElement",
                  1,
                  "RenderScript", RenderScriptVar);
  genBuildElement(C, "eb", ERT, RenderScriptVar, /* IsInline = */false);
  C.endFunction();

  C.startFunction(Context::AM_Public,
                  false,
                  NULL,
                  C.getClassName(),
                  2,
                  "RenderScript", RenderScriptVar,
                  "int", "count");

  C.indent() << RS_TYPE_ITEM_BUFFER_NAME" = null;" << std::endl;
  C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME" = null;" << std::endl;
  C.indent() << "mElement = createElement(" << RenderScriptVar << ");"
             << std::endl;
  // Call init() in super class
  C.indent() << "init(" << RenderScriptVar << ", count);" << std::endl;
  C.endFunction();

  C.startFunction(Context::AM_Public,
                  false,
                  NULL,
                  C.getClassName(),
                  3,
                  "RenderScript", RenderScriptVar,
                  "int", "count",
                  "int", "usages");

  C.indent() << RS_TYPE_ITEM_BUFFER_NAME" = null;" << std::endl;
  C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME" = null;" << std::endl;
  C.indent() << "mElement = createElement(" << RenderScriptVar << ");"
             << std::endl;
  // Call init() in super class
  C.indent() << "init(" << RenderScriptVar << ", count, usages);" << std::endl;
  C.endFunction();

  return;
}

void RSReflection::genTypeClassCopyToArray(Context &C,
                                           const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_Private,
                  false,
                  "void",
                  "copyToArray",
                  2,
                  RS_TYPE_ITEM_CLASS_NAME, "i",
                  "int", "index");

  genNewItemBufferPackerIfNull(C);
  C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME
                ".reset(index * "RS_TYPE_ITEM_CLASS_NAME".sizeof);"
             << std::endl;

  C.indent() << "copyToArrayLocal(i, " RS_TYPE_ITEM_BUFFER_PACKER_NAME
                ");" << std::endl;

  C.endFunction();
  return;
}

void RSReflection::genTypeClassCopyToArrayLocal(Context &C,
                                                const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_Private,
                  false,
                  "void",
                  "copyToArrayLocal",
                  2,
                  RS_TYPE_ITEM_CLASS_NAME, "i",
                  "FieldPacker", "fp");

  genPackVarOfType(C, ERT, "i", "fp");

  C.endFunction();
  return;
}

void RSReflection::genTypeClassItemSetter(Context &C,
                                          const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_PublicSynchronized,
                  false,
                  "void",
                  "set",
                  3,
                  RS_TYPE_ITEM_CLASS_NAME, "i",
                  "int", "index",
                  "boolean", "copyNow");
  genNewItemBufferIfNull(C, NULL);
  C.indent() << RS_TYPE_ITEM_BUFFER_NAME"[index] = i;" << std::endl;

  C.indent() << "if (copyNow) ";
  C.startBlock();

  C.indent() << "copyToArray(i, index);" << std::endl;
  C.indent() << "FieldPacker fp = new FieldPacker(" RS_TYPE_ITEM_CLASS_NAME
                ".sizeof);" << std::endl;
  C.indent() << "copyToArrayLocal(i, fp);" << std::endl;
  C.indent() << "mAllocation.setFromFieldPacker(index, fp);" << std::endl;

  // End of if (copyNow)
  C.endBlock();

  C.endFunction();
  return;
}

void RSReflection::genTypeClassItemGetter(Context &C,
                                          const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_PublicSynchronized,
                  false,
                  RS_TYPE_ITEM_CLASS_NAME,
                  "get",
                  1,
                  "int", "index");
  C.indent() << "if ("RS_TYPE_ITEM_BUFFER_NAME" == null) return null;"
             << std::endl;
  C.indent() << "return "RS_TYPE_ITEM_BUFFER_NAME"[index];" << std::endl;
  C.endFunction();
  return;
}

void RSReflection::genTypeClassComponentSetter(Context &C,
                                               const RSExportRecordType *ERT) {
  for (RSExportRecordType::const_field_iterator FI = ERT->fields_begin(),
           FE = ERT->fields_end();
       FI != FE;
       FI++) {
    const RSExportRecordType::Field *F = *FI;
    size_t FieldOffset = F->getOffsetInParent();
    size_t FieldStoreSize = RSExportType::GetTypeStoreSize(F->getType());
    unsigned FieldIndex = C.getFieldIndex(F);

    C.startFunction(Context::AM_PublicSynchronized,
                    false,
                    "void",
                    "set_" + F->getName(), 3,
                    "int", "index",
                    GetTypeName(F->getType()).c_str(), "v",
                    "boolean", "copyNow");
    genNewItemBufferPackerIfNull(C);
    genNewItemBufferIfNull(C, "index");
    C.indent() << RS_TYPE_ITEM_BUFFER_NAME"[index]." << F->getName()
               << " = v;" << std::endl;

    C.indent() << "if (copyNow) ";
    C.startBlock();

    if (FieldOffset > 0)
      C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME
                    ".reset(index * "RS_TYPE_ITEM_CLASS_NAME".sizeof + "
                 << FieldOffset << ");" << std::endl;
    else
      C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME
                    ".reset(index * "RS_TYPE_ITEM_CLASS_NAME".sizeof);"
                 << std::endl;
    genPackVarOfType(C, F->getType(), "v", RS_TYPE_ITEM_BUFFER_PACKER_NAME);

    C.indent() << "FieldPacker fp = new FieldPacker(" << FieldStoreSize << ");"
               << std::endl;
    genPackVarOfType(C, F->getType(), "v", "fp");
    C.indent() << "mAllocation.setFromFieldPacker(index, " << FieldIndex
               << ", fp);"
               << std::endl;

    // End of if (copyNow)
    C.endBlock();

    C.endFunction();
  }
  return;
}

void RSReflection::genTypeClassComponentGetter(Context &C,
                                               const RSExportRecordType *ERT) {
  for (RSExportRecordType::const_field_iterator FI = ERT->fields_begin(),
           FE = ERT->fields_end();
       FI != FE;
       FI++) {
    const RSExportRecordType::Field *F = *FI;
    C.startFunction(Context::AM_PublicSynchronized,
                    false,
                    GetTypeName(F->getType()).c_str(),
                    "get_" + F->getName(),
                    1,
                    "int", "index");
    C.indent() << "if ("RS_TYPE_ITEM_BUFFER_NAME" == null) return "
               << GetTypeNullValue(F->getType()) << ";" << std::endl;
    C.indent() << "return "RS_TYPE_ITEM_BUFFER_NAME"[index]." << F->getName()
               << ";" << std::endl;
    C.endFunction();
  }
  return;
}

void RSReflection::genTypeClassCopyAll(Context &C,
                                       const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_PublicSynchronized, false, "void", "copyAll", 0);

  C.indent() << "for (int ct = 0; ct < "RS_TYPE_ITEM_BUFFER_NAME".length; ct++)"
                  " copyToArray("RS_TYPE_ITEM_BUFFER_NAME"[ct], ct);"
             << std::endl;
  C.indent() << "mAllocation.setFromFieldPacker(0, "
                  RS_TYPE_ITEM_BUFFER_PACKER_NAME");"
             << std::endl;

  C.endFunction();
  return;
}

void RSReflection::genTypeClassResize(Context &C) {
  C.startFunction(Context::AM_PublicSynchronized,
                  false,
                  "void",
                  "resize",
                  1,
                  "int", "newSize");

  C.indent() << "if (mItemArray != null) ";
  C.startBlock();
  C.indent() << "int oldSize = mItemArray.length;" << std::endl;
  C.indent() << "int copySize = Math.min(oldSize, newSize);" << std::endl;
  C.indent() << "if (newSize == oldSize) return;" << std::endl;
  C.indent() << "Item ni[] = new Item[newSize];" << std::endl;
  C.indent() << "System.arraycopy(mItemArray, 0, ni, 0, copySize);"
             << std::endl;
  C.indent() << "mItemArray = ni;" << std::endl;
  C.endBlock();
  C.indent() << "mAllocation.resize(newSize);" << std::endl;

  C.indent() << "if (" RS_TYPE_ITEM_BUFFER_PACKER_NAME " != null) "
                  RS_TYPE_ITEM_BUFFER_PACKER_NAME " = "
                    "new FieldPacker(" RS_TYPE_ITEM_CLASS_NAME
                      ".sizeof * getType().getX()/* count */"
                        ");" << std::endl;

  C.endFunction();
  return;
}

/******************** Methods to generate type class /end ********************/

/********** Methods to create Element in Java of given record type ***********/
void RSReflection::genBuildElement(Context &C,
                                   const char *ElementBuilderName,
                                   const RSExportRecordType *ERT,
                                   const char *RenderScriptVar,
                                   bool IsInline) {
  C.indent() << "Element.Builder " << ElementBuilderName << " = "
      "new Element.Builder(" << RenderScriptVar << ");" << std::endl;

  // eb.add(...)
  genAddElementToElementBuilder(C,
                                ERT,
                                "",
                                ElementBuilderName,
                                RenderScriptVar,
                                /* ArraySize = */0);

  if (!IsInline)
    C.indent() << "return " << ElementBuilderName << ".create();" << std::endl;
  return;
}

#define EB_ADD(x) do {                                              \
  C.indent() << ElementBuilderName                                  \
             << ".add(" << x << ", \"" << VarName << "\"";  \
  if (ArraySize > 0)                                                \
    C.out() << ", " << ArraySize;                                   \
  C.out() << ");" << std::endl;                                     \
  C.incFieldIndex();                                                \
} while (false)

void RSReflection::genAddElementToElementBuilder(Context &C,
                                                 const RSExportType *ET,
                                                 const std::string &VarName,
                                                 const char *ElementBuilderName,
                                                 const char *RenderScriptVar,
                                                 unsigned ArraySize) {
  const char *ElementConstruct = GetBuiltinElementConstruct(ET);

  if (ElementConstruct != NULL) {
    EB_ADD(ElementConstruct << "(" << RenderScriptVar << ")");
  } else {
    if ((ET->getClass() == RSExportType::ExportClassPrimitive) ||
        (ET->getClass() == RSExportType::ExportClassVector)) {
      const RSExportPrimitiveType *EPT =
          static_cast<const RSExportPrimitiveType*>(ET);
      const char *DataKindName = GetElementDataKindName(EPT->getKind());
      const char *DataTypeName = GetElementDataTypeName(EPT->getType());
      int Size = (ET->getClass() == RSExportType::ExportClassVector) ?
          static_cast<const RSExportVectorType*>(ET)->getNumElement() :
          1;

      switch (EPT->getKind()) {
        case RSExportPrimitiveType::DataKindPixelL:
        case RSExportPrimitiveType::DataKindPixelA:
        case RSExportPrimitiveType::DataKindPixelLA:
        case RSExportPrimitiveType::DataKindPixelRGB:
        case RSExportPrimitiveType::DataKindPixelRGBA: {
          // Element.createPixel()
          EB_ADD("Element.createPixel(" << RenderScriptVar << ", "
                                        << DataTypeName << ", "
                                        << DataKindName << ")");
          break;
        }
        case RSExportPrimitiveType::DataKindUser:
        default: {
          if (EPT->getClass() == RSExportType::ExportClassPrimitive) {
            // Element.createUser()
            EB_ADD("Element.createUser(" << RenderScriptVar << ", "
                                         << DataTypeName << ")");
          } else {
            slangAssert((ET->getClass() == RSExportType::ExportClassVector) &&
                        "Unexpected type.");
            EB_ADD("Element.createVector(" << RenderScriptVar << ", "
                                           << DataTypeName << ", "
                                           << Size << ")");
          }
          break;
        }
      }
#ifndef NDEBUG
    } else if (ET->getClass() == RSExportType::ExportClassPointer) {
      // Pointer type variable should be resolved in
      // GetBuiltinElementConstruct()
      slangAssert(false && "??");
    } else if (ET->getClass() == RSExportType::ExportClassMatrix) {
      // Matrix type variable should be resolved
      // in GetBuiltinElementConstruct()
      slangAssert(false && "??");
#endif
    } else if (ET->getClass() == RSExportType::ExportClassConstantArray) {
      const RSExportConstantArrayType *ECAT =
          static_cast<const RSExportConstantArrayType *>(ET);

      const RSExportType *ElementType = ECAT->getElementType();
      if (ElementType->getClass() != RSExportType::ExportClassRecord) {
        genAddElementToElementBuilder(C,
                                      ECAT->getElementType(),
                                      VarName,
                                      ElementBuilderName,
                                      RenderScriptVar,
                                      ECAT->getSize());
      } else {
        std::string NewElementBuilderName(ElementBuilderName);
        NewElementBuilderName.append(1, '_');

        genBuildElement(C,
                        NewElementBuilderName.c_str(),
                        static_cast<const RSExportRecordType*>(ElementType),
                        RenderScriptVar,
                        /* IsInline = */true);
        ArraySize = ECAT->getSize();
        EB_ADD(NewElementBuilderName << ".create()");
      }
    } else if (ET->getClass() == RSExportType::ExportClassRecord) {
      // Simalar to case of RSExportType::ExportClassRecord in genPackVarOfType.
      //
      // TODO(zonr): Generalize these two function such that there's no
      //             duplicated codes.
      const RSExportRecordType *ERT =
          static_cast<const RSExportRecordType*>(ET);
      int Pos = 0;    // relative pos from now on

      for (RSExportRecordType::const_field_iterator I = ERT->fields_begin(),
               E = ERT->fields_end();
           I != E;
           I++) {
        const RSExportRecordType::Field *F = *I;
        std::string FieldName;
        int FieldOffset = F->getOffsetInParent();
        int FieldStoreSize = RSExportType::GetTypeStoreSize(F->getType());
        int FieldAllocSize = RSExportType::GetTypeAllocSize(F->getType());

        if (!VarName.empty())
          FieldName = VarName + "." + F->getName();
        else
          FieldName = F->getName();

        // Alignment
        genAddPaddingToElementBuiler(C,
                                     (FieldOffset - Pos),
                                     ElementBuilderName,
                                     RenderScriptVar);

        // eb.add(...)
        C.addFieldIndexMapping(F);
        if (F->getType()->getClass() != RSExportType::ExportClassRecord) {
          genAddElementToElementBuilder(C,
                                        F->getType(),
                                        FieldName,
                                        ElementBuilderName,
                                        RenderScriptVar,
                                        0);
        } else {
          std::string NewElementBuilderName(ElementBuilderName);
          NewElementBuilderName.append(1, '_');

          genBuildElement(C,
                          NewElementBuilderName.c_str(),
                          static_cast<const RSExportRecordType*>(F->getType()),
                          RenderScriptVar,
                          /* IsInline = */true);

          const std::string &VarName = FieldName;  // Hack for EB_ADD macro
          EB_ADD(NewElementBuilderName << ".create()");
        }

        // There is padding within the field type
        genAddPaddingToElementBuiler(C,
                                     (FieldAllocSize - FieldStoreSize),
                                     ElementBuilderName,
                                     RenderScriptVar);

        Pos = FieldOffset + FieldAllocSize;
      }

      // There maybe some padding after the struct
      size_t RecordAllocSize = RSExportType::GetTypeAllocSize(ERT);

      genAddPaddingToElementBuiler(C,
                                   RecordAllocSize - Pos,
                                   ElementBuilderName,
                                   RenderScriptVar);
    } else {
      slangAssert(false && "Unknown class of type");
    }
  }
}

void RSReflection::genAddPaddingToElementBuiler(Context &C,
                                                int PaddingSize,
                                                const char *ElementBuilderName,
                                                const char *RenderScriptVar) {
  unsigned ArraySize = 0;   // Hack the EB_ADD macro
  while (PaddingSize > 0) {
    const std::string &VarName = C.createPaddingField();
    if (PaddingSize >= 4) {
      EB_ADD("Element.U32(" << RenderScriptVar << ")");
      PaddingSize -= 4;
    } else if (PaddingSize >= 2) {
      EB_ADD("Element.U16(" << RenderScriptVar << ")");
      PaddingSize -= 2;
    } else if (PaddingSize >= 1) {
      EB_ADD("Element.U8(" << RenderScriptVar << ")");
      PaddingSize -= 1;
    }
  }
  return;
}

#undef EB_ADD
/******** Methods to create Element in Java of given record type /end ********/

bool RSReflection::reflect(const std::string &OutputPathBase,
                           const std::string &OutputPackageName,
                           const std::string &InputFileName,
                           const std::string &OutputBCFileName) {
  Context *C = NULL;
  std::string ResourceId = "";

  if (!GetClassNameFromFileName(OutputBCFileName, ResourceId))
    return false;

  if (ResourceId.empty())
    ResourceId = "<Resource ID>";

  if (OutputPackageName.empty() || OutputPackageName == "-")
    C = new Context(OutputPathBase, InputFileName, "<Package Name>",
                    ResourceId, true);
  else
    C = new Context(OutputPathBase, InputFileName, OutputPackageName,
                    ResourceId, false);

  if (C != NULL) {
    std::string ErrorMsg, ScriptClassName;
    // class ScriptC_<ScriptName>
    if (!GetClassNameFromFileName(InputFileName, ScriptClassName))
      return false;

    if (ScriptClassName.empty())
      ScriptClassName = "<Input Script Name>";

    ScriptClassName.insert(0, RS_SCRIPT_CLASS_NAME_PREFIX);

    if (mRSContext->getLicenseNote() != NULL) {
      C->setLicenseNote(*(mRSContext->getLicenseNote()));
    }

    if (!genScriptClass(*C, ScriptClassName, ErrorMsg)) {
      std::cerr << "Failed to generate class " << ScriptClassName << " ("
                << ErrorMsg << ")" << std::endl;
      return false;
    }

    mGeneratedFileNames->push_back(ScriptClassName);

    // class ScriptField_<TypeName>
    for (RSContext::const_export_type_iterator TI =
             mRSContext->export_types_begin(),
             TE = mRSContext->export_types_end();
         TI != TE;
         TI++) {
      const RSExportType *ET = TI->getValue();

      if (ET->getClass() == RSExportType::ExportClassRecord) {
        const RSExportRecordType *ERT =
            static_cast<const RSExportRecordType*>(ET);

        if (!ERT->isArtificial() && !genTypeClass(*C, ERT, ErrorMsg)) {
          std::cerr << "Failed to generate type class for struct '"
                    << ERT->getName() << "' (" << ErrorMsg << ")" << std::endl;
          return false;
        }
      }
    }
  }

  return true;
}

/************************** RSReflection::Context **************************/
const char *const RSReflection::Context::ApacheLicenseNote =
    "/*\n"
    " * Copyright (C) 2011 The Android Open Source Project\n"
    " *\n"
    " * Licensed under the Apache License, Version 2.0 (the \"License\");\n"
    " * you may not use this file except in compliance with the License.\n"
    " * You may obtain a copy of the License at\n"
    " *\n"
    " *      http://www.apache.org/licenses/LICENSE-2.0\n"
    " *\n"
    " * Unless required by applicable law or agreed to in writing, software\n"
    " * distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    " * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or "
    "implied.\n"
    " * See the License for the specific language governing permissions and\n"
    " * limitations under the License.\n"
    " */\n"
    "\n";

const char *const RSReflection::Context::Import[] = {
  // Renderscript java class
  "android.renderscript.*",
  // Import R
  "android.content.res.Resources",
  // Import for debugging
  // "android.util.Log",
};

bool RSReflection::Context::openClassFile(const std::string &ClassName,
                                          std::string &ErrorMsg) {
  if (!mUseStdout) {
    mOF.clear();
    std::string Path =
        RSSlangReflectUtils::ComputePackagedPath(mOutputPathBase.c_str(),
                                                 mPackageName.c_str());

    if (!SlangUtils::CreateDirectoryWithParents(Path, &ErrorMsg))
      return false;

    std::string ClassFile = Path + OS_PATH_SEPARATOR_STR + ClassName + ".java";

    mOF.open(ClassFile.c_str());
    if (!mOF.good()) {
      ErrorMsg = "failed to open file '" + ClassFile + "' for write";
      return false;
    }
  }
  return true;
}

const char *RSReflection::Context::AccessModifierStr(AccessModifier AM) {
  switch (AM) {
    case AM_Public: return "public"; break;
    case AM_Protected: return "protected"; break;
    case AM_Private: return "private"; break;
    case AM_PublicSynchronized: return "public synchronized"; break;
    default: return ""; break;
  }
}

bool RSReflection::Context::startClass(AccessModifier AM,
                                       bool IsStatic,
                                       const std::string &ClassName,
                                       const char *SuperClassName,
                                       std::string &ErrorMsg) {
  if (mVerbose)
    std::cout << "Generating " << ClassName << ".java ..." << std::endl;

  // Open file for class
  if (!openClassFile(ClassName, ErrorMsg))
    return false;

  // License
  out() << mLicenseNote;

  // Notice of generated file
  out() << "/*" << std::endl;
  out() << " * This file is auto-generated. DO NOT MODIFY!" << std::endl;
  out() << " * The source Renderscript file: " << mInputRSFile << std::endl;
  out() << " */" << std::endl;

  // Package
  if (!mPackageName.empty())
    out() << "package " << mPackageName << ";" << std::endl;
  out() << std::endl;

  // Imports
  for (unsigned i = 0; i < (sizeof(Import) / sizeof(const char*)); i++)
    out() << "import " << Import[i] << ";" << std::endl;
  out() << std::endl;

  // All reflected classes should be annotated as hidden, so that they won't
  // be exposed in SDK.
  out() << "/**" << std::endl;
  out() << " * @hide" << std::endl;
  out() << " */" << std::endl;

  out() << AccessModifierStr(AM) << ((IsStatic) ? " static" : "") << " class "
        << ClassName;
  if (SuperClassName != NULL)
    out() << " extends " << SuperClassName;

  startBlock();

  mClassName = ClassName;

  return true;
}

void RSReflection::Context::endClass() {
  endBlock();
  if (!mUseStdout)
    mOF.close();
  clear();
  return;
}

void RSReflection::Context::startBlock(bool ShouldIndent) {
  if (ShouldIndent)
    indent() << "{" << std::endl;
  else
    out() << " {" << std::endl;
  incIndentLevel();
  return;
}

void RSReflection::Context::endBlock() {
  decIndentLevel();
  indent() << "}" << std::endl << std::endl;
  return;
}

void RSReflection::Context::startTypeClass(const std::string &ClassName) {
  indent() << "public static class " << ClassName;
  startBlock();
  return;
}

void RSReflection::Context::endTypeClass() {
  endBlock();
  return;
}

void RSReflection::Context::startFunction(AccessModifier AM,
                                          bool IsStatic,
                                          const char *ReturnType,
                                          const std::string &FunctionName,
                                          int Argc, ...) {
  ArgTy Args;
  va_list vl;
  va_start(vl, Argc);

  for (int i = 0; i < Argc; i++) {
    const char *ArgType = va_arg(vl, const char*);
    const char *ArgName = va_arg(vl, const char*);

    Args.push_back(std::make_pair(ArgType, ArgName));
  }
  va_end(vl);

  startFunction(AM, IsStatic, ReturnType, FunctionName, Args);

  return;
}

void RSReflection::Context::startFunction(AccessModifier AM,
                                          bool IsStatic,
                                          const char *ReturnType,
                                          const std::string &FunctionName,
                                          const ArgTy &Args) {
  indent() << AccessModifierStr(AM) << ((IsStatic) ? " static " : " ")
           << ((ReturnType) ? ReturnType : "") << " " << FunctionName << "(";

  bool FirstArg = true;
  for (ArgTy::const_iterator I = Args.begin(), E = Args.end();
       I != E;
       I++) {
    if (!FirstArg)
      out() << ", ";
    else
      FirstArg = false;

    out() << I->first << " " << I->second;
  }

  out() << ")";
  startBlock();

  return;
}

void RSReflection::Context::endFunction() {
  endBlock();
  return;
}

}  // namespace slang
