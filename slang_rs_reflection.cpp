#include "slang_rs_reflection.h"

#include <utility>
#include <cstdarg>
#include <cctype>
#include <sys/stat.h>

#include "llvm/ADT/APFloat.h"

#include "slang_rs_context.h"
#include "slang_rs_export_var.h"
#include "slang_rs_export_func.h"
#include "slang_rs_reflect_utils.h"

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

#define RS_EXPORT_VAR_ALLOCATION_PREFIX  "mAlloction_"
#define RS_EXPORT_VAR_DATA_STORAGE_PREFIX "mData_"

using namespace slang;

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

    "Matrix2f",     // RSExportPrimitiveType::DataTypeRSMatrix2x2
    "Matrix3f",     // RSExportPrimitiveType::DataTypeRSMatrix3x3
    "Matrix4f",     // RSExportPrimitiveType::DataTypeRSMatrix4x4

    "Element",  // RSExportPrimitiveType::DataTypeRSElement
    "Type",     // RSExportPrimitiveType::DataTypeRSType
    "Allocation",   // RSExportPrimitiveType::DataTypeRSAllocation
    "Sampler",  // RSExportPrimitiveType::DataTypeRSSampler
    "Script",   // RSExportPrimitiveType::DataTypeRSScript
    "Mesh",       // RSExportPrimitiveType::DataTypeRSMesh
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

  assert(false && "GetPrimitiveTypeName : Unknown primitive data type");
  return NULL;
}

static const char *GetVectorTypeName(const RSExportVectorType *EVT) {
  static const char *VectorTypeJavaNameMap[][3] = {
    /* 0 */ { "Byte2",  "Byte3",    "Byte4" },
    /* 1 */ { "Short2", "Short3",   "Short4" },
    /* 2 */ { "Int2",   "Int3",     "Int4" },
    /* 3 */ { "Long2",  "Long3",    "Long4" },
    /* 4 */ { "Float2", "Float3",   "Float4" },
  };

  const char **BaseElement = NULL;

  switch (EVT->getType()) {
    case RSExportPrimitiveType::DataTypeSigned8:
    case RSExportPrimitiveType::DataTypeBoolean: {
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
    case RSExportPrimitiveType::DataTypeUnsigned32: {
      BaseElement = VectorTypeJavaNameMap[3];
      break;
    }
    case RSExportPrimitiveType::DataTypeFloat32: {
      BaseElement = VectorTypeJavaNameMap[4];
      break;
    }
    default: {
      assert(false && "RSReflection::genElementTypeName : Unsupported vector "
                      "element data type");
      break;
    }
  }

  assert((EVT->getNumElement() > 1) &&
         (EVT->getNumElement() <= 4) &&
         "Number of element in vector type is invalid");

  return BaseElement[EVT->getNumElement() - 2];
}

static const char *GetVectorAccessor(int Index) {
  static const char *VectorAccessorMap[] = {
    /* 0 */ "x",
    /* 1 */ "y",
    /* 2 */ "z",
    /* 3 */ "w",
  };

  assert((Index >= 0) &&
         (Index < (sizeof(VectorAccessorMap) / sizeof(const char*))) &&
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

    "addObj",   // RSExportPrimitiveType::DataTypeRSMatrix2x2
    "addObj",   // RSExportPrimitiveType::DataTypeRSMatrix3x3
    "addObj",   // RSExportPrimitiveType::DataTypeRSMatrix4x4

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

  assert(false && "GetPackerAPIName : Unknown primitive data type");
  return NULL;
}

static std::string GetTypeName(const RSExportType *ET) {
  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive:
    case RSExportType::ExportClassConstantArray: {
      return GetPrimitiveTypeName(
                static_cast<const RSExportPrimitiveType*>(ET));
      break;
    }
    case RSExportType::ExportClassPointer: {
      const RSExportType *PointeeType =
          static_cast<const RSExportPointerType*>(ET)->getPointeeType();

      if (PointeeType->getClass() != RSExportType::ExportClassRecord)
        return "Allocation";
      else
        return RS_TYPE_CLASS_NAME_PREFIX + PointeeType->getName();
      break;
    }
    case RSExportType::ExportClassVector: {
      return GetVectorTypeName(static_cast<const RSExportVectorType*>(ET));
      break;
    }
    case RSExportType::ExportClassRecord: {
      return RS_TYPE_CLASS_NAME_PREFIX + ET->getName() +
                 "."RS_TYPE_ITEM_CLASS_NAME;
      break;
    }
    default: {
      assert(false && "Unknown class of type");
    }
  }

  return "";
}

static const char *GetBuiltinElementConstruct(const RSExportType *ET) {
  if (ET->getClass() == RSExportType::ExportClassPrimitive ||
      ET->getClass() == RSExportType::ExportClassConstantArray) {
    const RSExportPrimitiveType *EPT =
        static_cast<const RSExportPrimitiveType*>(ET);
    if (EPT->getKind() == RSExportPrimitiveType::DataKindUser) {
      static const char *PrimitiveBuiltinElementConstructMap[] = {
        NULL,   // RSExportPrimitiveType::DataTypeFloat16
        "F32",  // RSExportPrimitiveType::DataTypeFloat32
        "F64",  // RSExportPrimitiveType::DataTypeFloat64
        "I8",   // RSExportPrimitiveType::DataTypeSigned8
        NULL,   // RSExportPrimitiveType::DataTypeSigned16
        "I32",  // RSExportPrimitiveType::DataTypeSigned32
        "I64",  // RSExportPrimitiveType::DataTypeSigned64
        "U8",   // RSExportPrimitiveType::DataTypeUnsigned8
        NULL,   // RSExportPrimitiveType::DataTypeUnsigned16
        "U32",  // RSExportPrimitiveType::DataTypeUnsigned32
        NULL,   // RSExportPrimitiveType::DataTypeUnsigned64
        "BOOLEAN",  // RSExportPrimitiveType::DataTypeBoolean

        NULL,   // RSExportPrimitiveType::DataTypeUnsigned565
        NULL,   // RSExportPrimitiveType::DataTypeUnsigned5551
        NULL,   // RSExportPrimitiveType::DataTypeUnsigned4444

        "MATRIX_2X2",   // RSExportPrimitiveType::DataTypeRSMatrix2x2
        "MATRIX_3X3",   // RSExportPrimitiveType::DataTypeRSMatrix3x3
        "MATRIX_4X4",   // RSExportPrimitiveType::DataTypeRSMatrix4x4

        "ELEMENT",      // RSExportPrimitiveType::DataTypeRSElement
        "TYPE",         // RSExportPrimitiveType::DataTypeRSType
        "ALLOCATION",   // RSExportPrimitiveType::DataTypeRSAllocation
        "SAMPLER",      // RSExportPrimitiveType::DataTypeRSSampler
        "SCRIPT",       // RSExportPrimitiveType::DataTypeRSScript
        "MESH",         // RSExportPrimitiveType::DataTypeRSMesh
        "PROGRAM_FRAGMENT",  // RSExportPrimitiveType::DataTypeRSProgramFragment
        "PROGRAM_VERTEX",    // RSExportPrimitiveType::DataTypeRSProgramVertex
        "PROGRAM_RASTER",    // RSExportPrimitiveType::DataTypeRSProgramRaster
        "PROGRAM_STORE",     // RSExportPrimitiveType::DataTypeRSProgramStore
        "FONT",       // RSExportPrimitiveType::DataTypeRSFont
      };
      unsigned TypeId = EPT->getType();

      if (TypeId <
          (sizeof(PrimitiveBuiltinElementConstructMap) / sizeof(const char*)))
        return PrimitiveBuiltinElementConstructMap[ EPT->getType() ];
    } else if (EPT->getKind() == RSExportPrimitiveType::DataKindPixelA) {
      if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
        return "A_8";
    } else if (EPT->getKind() == RSExportPrimitiveType::DataKindPixelRGB) {
      if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned565)
        return "RGB_565";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
        return "RGB_888";
    } else if (EPT->getKind() == RSExportPrimitiveType::DataKindPixelRGBA) {
      if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned5551)
        return "RGBA_5551";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned4444)
        return "RGBA_4444";
      else if (EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
        return "RGBA_8888";
    }
  } else if (ET->getClass() == RSExportType::ExportClassVector) {
    const RSExportVectorType *EVT = static_cast<const RSExportVectorType*>(ET);
    if (EVT->getKind() == RSExportPrimitiveType::DataKindUser) {
      if (EVT->getType() == RSExportPrimitiveType::DataTypeFloat32) {
        if (EVT->getNumElement() == 2)
          return "F32_2";
        else if (EVT->getNumElement() == 3)
          return "F32_3";
        else if (EVT->getNumElement() == 4)
          return "F32_4";
      } else if (EVT->getType() == RSExportPrimitiveType::DataTypeUnsigned8) {
        if (EVT->getNumElement() == 4)
          return "U8_4";
      }
    }
  } else if (ET->getClass() == RSExportType::ExportClassPointer) {
    // Treat pointer type variable as unsigned int
    // TODO(zonr): this is target dependent
    return "USER_I32";
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
    NULL,                           // RSExportPrimitiveType::DataTypeFloat16
    "Element.DataType.FLOAT_32",    // RSExportPrimitiveType::DataTypeFloat32
    "Element.DataType.FLOAT_64",    // RSExportPrimitiveType::DataTypeFloat64
    "Element.DataType.SIGNED_8",    // RSExportPrimitiveType::DataTypeSigned8
    "Element.DataType.SIGNED_16",   // RSExportPrimitiveType::DataTypeSigned16
    "Element.DataType.SIGNED_32",   // RSExportPrimitiveType::DataTypeSigned32
    "Element.DataType.SIGNED_64",   // RSExportPrimitiveType::DataTypeSigned64
    "Element.DataType.UNSIGNED_8",  // RSExportPrimitiveType::DataTypeUnsigned8
    "Element.DataType.UNSIGNED_16", // RSExportPrimitiveType::DataTypeUnsigned16
    "Element.DataType.UNSIGNED_32", // RSExportPrimitiveType::DataTypeUnsigned32
    NULL,                           // RSExportPrimitiveType::DataTypeUnsigned64
    "Element.DataType.BOOLEAN",     // RSExportPrimitiveType::DataTypeBoolean

    // RSExportPrimitiveType::DataTypeUnsigned565
    "Element.DataType.UNSIGNED_5_6_5",
    // RSExportPrimitiveType::DataTypeUnsigned5551
    "Element.DataType.UNSIGNED_5_5_5_1",
    // RSExportPrimitiveType::DataTypeUnsigned4444
    "Element.DataType.UNSIGNED_4_4_4_4",

    // RSExportPrimitiveType::DataTypeRSMatrix2x2
    "Element.DataType.RS_MATRIX_2X2",
    // RSExportPrimitiveType::DataTypeRSMatrix3x3
    "Element.DataType.RS_MATRIX_3X3",
    // RSExportPrimitiveType::DataTypeRSMatrix4x4
    "Element.DataType.RS_MATRIX_4X4",

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

bool RSReflection::openScriptFile(Context &C,
                                  const std::string &ClassName,
                                  std::string &ErrorMsg) {
  if (!C.mUseStdout) {
    C.mOF.clear();
    std::string _path = RSSlangReflectUtils::ComputePackagedPath(
        mRSContext->getReflectJavaPathName().c_str(),
        C.getPackageName().c_str());

    RSSlangReflectUtils::mkdir_p(_path.c_str());
    C.mOF.open((_path + "/" + ClassName + ".java").c_str());
    if (!C.mOF.good()) {
      ErrorMsg = "failed to open file '" + _path + "/" + ClassName
          + ".java' for write";

      return false;
    }
  }
  return true;
}

/********************** Methods to generate script class **********************/
bool RSReflection::genScriptClass(Context &C,
                                  const std::string &ClassName,
                                  std::string &ErrorMsg) {
  // Open the file
  if (!openScriptFile(C, ClassName, ErrorMsg)) {
    return false;
  }

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
                  4,
                  "RenderScript",
                  "rs",
                  "Resources",
                  "resources",
                  "int",
                  "id",
                  "boolean",
                  "isRoot");
  // Call constructor of super class
  C.indent() << "super(rs, resources, id, isRoot);" << std::endl;

  // If an exported variable has initial value, reflect it

  for (RSContext::const_export_var_iterator I = mRSContext->export_vars_begin(),
           E = mRSContext->export_vars_end();
       I != E;
       I++) {
    const RSExportVar *EV = *I;
    if (!EV->getInit().isUninit())
      genInitExportVariable(C, EV->getType(), EV->getName(), EV->getInit());
  }

  C.endFunction();
  return;
}

void RSReflection::genInitBoolExportVariable(Context &C,
                                             const std::string &VarName,
                                             const clang::APValue &Val) {
  assert(!Val.isUninit() && "Not a valid initializer");

  C.indent() << RS_EXPORT_VAR_PREFIX << VarName << " = ";
  assert((Val.getKind() == clang::APValue::Int) &&
         "Bool type has wrong initial APValue");

  if (Val.getInt().getSExtValue() == 0) {
    C.out() << "false";
  } else {
    C.out() << "true";
  }
  C.out() << ";" << std::endl;

  return;
}

void RSReflection::genInitPrimitiveExportVariable(Context &C,
                                                  const std::string &VarName,
                                                  const clang::APValue &Val) {
  assert(!Val.isUninit() && "Not a valid initializer");

  C.indent() << RS_EXPORT_VAR_PREFIX << VarName << " = ";
  switch (Val.getKind()) {
    case clang::APValue::Int: {
      C.out() << Val.getInt().getSExtValue();
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
      assert(false && "Primitive type cannot have such kind of initializer");
      break;
    }
    default: {
      assert(false && "Unknown kind of initializer");
    }
  }
  C.out() << ";" << std::endl;

  return;
}

void RSReflection::genInitExportVariable(Context &C,
                                         const RSExportType *ET,
                                         const std::string &VarName,
                                         const clang::APValue &Val) {
  assert(!Val.isUninit() && "Not a valid initializer");

  switch (ET->getClass()) {
    case RSExportType::ExportClassPrimitive:
    case RSExportType::ExportClassConstantArray: {
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
          for (int i = 0; i < EVT->getNumElement(); i++) {
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
          assert(false && "Unexpected type of value of initializer.");
        }
      }
      break;
    }

    // TODO(zonr): Resolving initializer of a record type variable is complex.
    // It cannot obtain by just simply evaluating the initializer expression.
    case RSExportType::ExportClassRecord: {
#if 0
      unsigned InitIndex = 0;
      const RSExportRecordType *ERT =
          static_cast<const RSExportRecordType*>(ET);

      assert((Val.getKind() == clang::APValue::Vector) && "Unexpected type of "
             "initializer for record type variable");

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
      assert(false && "Unsupported initializer for record type variable "
                      "currently");
      break;
    }

    default: {
      assert(false && "Unknown class of type");
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
    case RSExportType::ExportClassPrimitive:
    case RSExportType::ExportClassConstantArray: {
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

    case RSExportType::ExportClassRecord: {
      genRecordTypeExportVariable(C, EV);
      break;
    }

    default: {
      assert(false && "Unknown class of type");
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

  for (RSExportFunc::const_param_iterator I = EF->params_begin(),
           E = EF->params_end();
       I != E;
       I++) {
    const RSExportFunc::Parameter *P = *I;
    Args.push_back(make_pair(GetTypeName(P->getType()), P->getName()));
  }

  C.startFunction(Context::AM_Public,
                  false,
                  "void",
                  "invoke_" + EF->getName(),
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

void RSReflection::genPrimitiveTypeExportVariable(Context &C,
                                                  const RSExportVar *EV) {
  assert((EV->getType()->getClass() == RSExportType::ExportClassPrimitive ||
          EV->getType()->getClass() == RSExportType::ExportClassConstantArray)
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
                    TypeName,
                    "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << std::endl;

    if (EPT->isRSObjectType())
      C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName()
                 << ", (v == null) ? 0 : v.getID());" << std::endl;
    else
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

  assert((ET->getClass() == RSExportType::ExportClassPointer) &&
         "Variable should be type of pointer here");

  PointeeType = static_cast<const RSExportPointerType*>(ET)->getPointeeType();
  TypeName = GetTypeName(ET);

  // bind_*()
  C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX
             << EV->getName() << ";" << std::endl;

  C.startFunction(Context::AM_Public,
                  false,
                  "void",
                  "bind_" + EV->getName(),
                  1,
                  TypeName.c_str(),
                  "v");

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
  assert((EV->getType()->getClass() == RSExportType::ExportClassVector) &&
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
                    TypeName,
                    "v");
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

void RSReflection::genRecordTypeExportVariable(Context &C,
                                               const RSExportVar *EV) {
  assert((EV->getType()->getClass() == RSExportType::ExportClassRecord) &&
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
                    TypeName.c_str(),
                    "v");
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
    case RSExportType::ExportClassConstantArray: {
      if (ET->getName().compare("addObj") == 0) {
        C.indent() << FieldPackerName << "." << "addObj" << "("
                   << VarName << ");" << std::endl;
      } else {
        C.indent() << FieldPackerName << "."
                   << GetPackerAPIName(
                       static_cast<const RSExportPrimitiveType*>(ET))
                   << "(" << VarName << ");" << std::endl;
      }
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
      size_t Padding = RSExportType::GetTypeAllocSize(ERT) - Pos;
      if (Padding > 0)
        C.indent() << FieldPackerName << ".skip(" << Padding << ");"
                   << std::endl;
      break;
    }
    default: {
      assert(false && "Unknown class of type");
    }
  }

  return;
}

void RSReflection::genNewItemBufferIfNull(Context &C, const char *Index) {
  C.indent() << "if ("RS_TYPE_ITEM_BUFFER_NAME" == null) "
      RS_TYPE_ITEM_BUFFER_NAME" = "
      "new "RS_TYPE_ITEM_CLASS_NAME"[mType.getX() /* count */];"
             << std::endl;
  if (Index != NULL)
    C.indent() << "if ("RS_TYPE_ITEM_BUFFER_NAME"[" << Index << "] == null) "
        RS_TYPE_ITEM_BUFFER_NAME"[" << Index << "] = "
        "new "RS_TYPE_ITEM_CLASS_NAME"();" << std::endl;
  return;
}

void RSReflection::genNewItemBufferPackerIfNull(Context &C) {
  C.indent() << "if ("RS_TYPE_ITEM_BUFFER_PACKER_NAME" == null) "
      RS_TYPE_ITEM_BUFFER_PACKER_NAME" = "
      "new FieldPacker("
      RS_TYPE_ITEM_CLASS_NAME".sizeof * mType.getX() /* count */);"
             << std::endl;
  return;
}

/********************** Methods to generate type class  **********************/
bool RSReflection::genTypeClass(Context &C,
                                const RSExportRecordType *ERT,
                                std::string &ErrorMsg) {
  std::string ClassName = RS_TYPE_CLASS_NAME_PREFIX + ERT->getName();

  // Open the file
  if (!openScriptFile(C, ClassName, ErrorMsg)) {
    return false;
  }

  if (!C.startClass(Context::AM_Public,
                    false,
                    ClassName,
                    RS_TYPE_CLASS_SUPER_CLASS_NAME,
                    ErrorMsg))
    return false;

  if (!genTypeItemClass(C, ERT, ErrorMsg))
    return false;

  // Declare item buffer and item buffer packer
  C.indent() << "private "RS_TYPE_ITEM_CLASS_NAME" "RS_TYPE_ITEM_BUFFER_NAME"[]"
      ";" << std::endl;
  C.indent() << "private FieldPacker "RS_TYPE_ITEM_BUFFER_PACKER_NAME";"
             << std::endl;

  genTypeClassConstructor(C, ERT);
  genTypeClassCopyToArray(C, ERT);
  genTypeClassItemSetter(C, ERT);
  genTypeClassItemGetter(C, ERT);
  genTypeClassComponentSetter(C, ERT);
  genTypeClassComponentGetter(C, ERT);
  genTypeClassCopyAll(C, ERT);

  C.endClass();

  C.resetFieldIndex();
  C.clearFieldIndexMap();

  return true;
}

bool RSReflection::genTypeItemClass(Context &C,
                                    const RSExportRecordType *ERT,
                                    std::string &ErrorMsg) {
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
    if ((F->getType()->getClass() == RSExportType::ExportClassVector) ||
        (F->getType()->getClass() == RSExportType::ExportClassRecord) ||
        (F->getType()->getClass() == RSExportType::ExportClassConstantArray)) {
      C.indent() << F->getName() << " = new " << GetTypeName(F->getType())
                 << "();" << std::endl;
    }
  }

  // end Constructor
  C.endBlock();

  // end Item class
  C.endBlock();

  return true;
}

void RSReflection::genTypeClassConstructor(Context &C,
                                           const RSExportRecordType *ERT) {
  const char *RenderScriptVar = "rs";

  C.startFunction(Context::AM_Public,
                  true,
                  "Element",
                  "createElement",
                  1,
                  "RenderScript",
                  RenderScriptVar);
  genBuildElement(C, ERT, RenderScriptVar);
  C.endFunction();

  C.startFunction(Context::AM_Public,
                  false,
                  NULL,
                  C.getClassName(),
                  2,
                  "RenderScript",
                  RenderScriptVar,
                  "int",
                  "count");

  C.indent() << RS_TYPE_ITEM_BUFFER_NAME" = null;" << std::endl;
  C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME" = null;" << std::endl;
  C.indent() << "mElement = createElement(" << RenderScriptVar << ");"
             << std::endl;
  // Call init() in super class
  C.indent() << "init(" << RenderScriptVar << ", count);" << std::endl;
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
                  RS_TYPE_ITEM_CLASS_NAME,
                  "i",
                  "int",
                  "index");

  genNewItemBufferPackerIfNull(C);
  C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME
      ".reset(index * "RS_TYPE_ITEM_CLASS_NAME".sizeof);" << std::endl;

  genPackVarOfType(C, ERT, "i", RS_TYPE_ITEM_BUFFER_PACKER_NAME);

  C.endFunction();
  return;
}

void RSReflection::genTypeClassItemSetter(Context &C,
                                          const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_Public,
                  false,
                  "void",
                  "set",
                  3,
                  RS_TYPE_ITEM_CLASS_NAME,
                  "i",
                  "int",
                  "index",
                  "boolean",
                  "copyNow");
  genNewItemBufferIfNull(C, NULL);
  C.indent() << RS_TYPE_ITEM_BUFFER_NAME"[index] = i;" << std::endl;

  C.indent() << "if (copyNow) ";
  C.startBlock();

  C.indent() << "copyToArray(i, index);" << std::endl;
  C.indent() << "mAllocation.subData1D(index, 1, "
      RS_TYPE_ITEM_BUFFER_PACKER_NAME".getData());" << std::endl;

  // End of if (copyNow)
  C.endBlock();

  C.endFunction();
  return;
}

void RSReflection::genTypeClassItemGetter(Context &C,
                                          const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_Public,
                  false,
                  RS_TYPE_ITEM_CLASS_NAME,
                  "get",
                  1,
                  "int",
                  "index");
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

    C.startFunction(Context::AM_Public,
                    false,
                    "void",
                    "set_" + F->getName(), 3,
                    "int",
                    "index",
                    GetTypeName(F->getType()).c_str(),
                    "v",
                    "boolean",
                    "copyNow");
    genNewItemBufferPackerIfNull(C);
    genNewItemBufferIfNull(C, "index");
    C.indent() << RS_TYPE_ITEM_BUFFER_NAME"[index]." << F->getName()
               << " = v;" << std::endl;

    C.indent() << "if (copyNow) ";
    C.startBlock();

    if (FieldOffset > 0)
      C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME".reset(index * "
          RS_TYPE_ITEM_CLASS_NAME".sizeof + " << FieldOffset << ");"
                 << std::endl;
    else
      C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME".reset(index * "
          RS_TYPE_ITEM_CLASS_NAME".sizeof);" << std::endl;
    genPackVarOfType(C, F->getType(), "v", RS_TYPE_ITEM_BUFFER_PACKER_NAME);

    C.indent() << "FieldPacker fp = new FieldPacker(" << FieldStoreSize << ");"
               << std::endl;
    genPackVarOfType(C, F->getType(), "v", "fp");
    C.indent() << "mAllocation.subElementData(index, " << FieldIndex << ", fp);"
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
    C.startFunction(Context::AM_Public,
                    false,
                    GetTypeName(F->getType()).c_str(),
                    "get_" + F->getName(),
                    1,
                    "int",
                    "index");
    C.indent() << "return "RS_TYPE_ITEM_BUFFER_NAME"[index]." << F->getName()
               << ";" << std::endl;
    C.endFunction();
  }
  return;
}

void RSReflection::genTypeClassCopyAll(Context &C,
                                       const RSExportRecordType *ERT) {
  C.startFunction(Context::AM_Public, false, "void", "copyAll", 0);

  C.indent() << "for (int ct=0; ct < "RS_TYPE_ITEM_BUFFER_NAME".length; ct++) "
      "copyToArray("RS_TYPE_ITEM_BUFFER_NAME"[ct], ct);"
             << std::endl;
  C.indent() << "mAllocation.data("RS_TYPE_ITEM_BUFFER_PACKER_NAME".getData());"
             << std::endl;

  C.endFunction();
  return;
}

/******************** Methods to generate type class /end ********************/

/********** Methods to create Element in Java of given record type ***********/
void RSReflection::genBuildElement(Context &C, const RSExportRecordType *ERT,
                                   const char *RenderScriptVar) {
  const char *ElementBuilderName = "eb";

  // Create element builder
  //    C.startBlock(true);

  C.indent() << "Element.Builder " << ElementBuilderName << " = "
      "new Element.Builder(" << RenderScriptVar << ");" << std::endl;

  // eb.add(...)
  genAddElementToElementBuilder(C,
                                ERT,
                                "",
                                ElementBuilderName,
                                RenderScriptVar);

  C.indent() << "return " << ElementBuilderName << ".create();" << std::endl;

  //   C.endBlock();
  return;
}

#define EB_ADD(x)                                                       \
  C.indent() << ElementBuilderName \
             << ".add(Element." << x << ", \"" << VarName << "\");" \
             << std::endl;                                               \
  C.incFieldIndex()

void RSReflection::genAddElementToElementBuilder(Context &C,
                                                 const RSExportType *ET,
                                                 const std::string &VarName,
                                                 const char *ElementBuilderName,
                                                 const char *RenderScriptVar) {
  const char *ElementConstruct = GetBuiltinElementConstruct(ET);

  if (ElementConstruct != NULL) {
    EB_ADD(ElementConstruct << "(" << RenderScriptVar << ")");
  } else {
    if ((ET->getClass() == RSExportType::ExportClassPrimitive) ||
        (ET->getClass() == RSExportType::ExportClassVector)    ||
        (ET->getClass() == RSExportType::ExportClassConstantArray)
        ) {
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
          EB_ADD("createPixel(" << RenderScriptVar << ", "
                                << DataTypeName << ", "
                                << DataKindName << ")");
          break;
        }
        case RSExportPrimitiveType::DataKindUser:
        default: {
          if (EPT->getClass() == RSExportType::ExportClassPrimitive ||
              EPT->getClass() == RSExportType::ExportClassConstantArray) {
            // Element.createUser()
            EB_ADD("createUser(" << RenderScriptVar << ", "
                                 << DataTypeName << ")");
          } else {
            // ET->getClass() == RSExportType::ExportClassVector must hold here
            // Element.createVector()
            EB_ADD("createVector(" << RenderScriptVar << ", "
                                   << DataTypeName << ", "
                                   << Size << ")");
          }
          break;
        }
      }
    } else if (ET->getClass() == RSExportType::ExportClassPointer) {
      // Pointer type variable should be resolved in
      // GetBuiltinElementConstruct()
      assert(false && "??");
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
        genAddElementToElementBuilder(C,
                                      F->getType(),
                                      FieldName,
                                      ElementBuilderName,
                                      RenderScriptVar);

        // There is padding within the field type
        genAddPaddingToElementBuiler(C,
                                     (FieldAllocSize - FieldStoreSize),
                                     ElementBuilderName,
                                     RenderScriptVar);

        Pos = FieldOffset + FieldAllocSize;
      }

      // There maybe some padding after the struct
      //unsigned char align = RSExportType::GetTypeAlignment(ERT);
      //size_t siz = RSExportType::GetTypeAllocSize(ERT);
      size_t siz1 = RSExportType::GetTypeStoreSize(ERT);

      genAddPaddingToElementBuiler(C,
                                   siz1 - Pos,
                                   ElementBuilderName,
                                   RenderScriptVar);
    } else {
      assert(false && "Unknown class of type");
    }
  }
}

void RSReflection::genAddPaddingToElementBuiler(Context &C,
                                                int PaddingSize,
                                                const char *ElementBuilderName,
                                                const char *RenderScriptVar) {
  while (PaddingSize > 0) {
    const std::string &VarName = C.createPaddingField();
    if (PaddingSize >= 4) {
      EB_ADD("U32(" << RenderScriptVar << ")");
      PaddingSize -= 4;
    } else if (PaddingSize >= 2) {
      EB_ADD("U16(" << RenderScriptVar << ")");
      PaddingSize -= 2;
    } else if (PaddingSize >= 1) {
      EB_ADD("U8(" << RenderScriptVar << ")");
      PaddingSize -= 1;
    }
  }
  return;
}

#undef EB_ADD
/******** Methods to create Element in Java of given record type /end ********/

bool RSReflection::reflect(const char *OutputPackageName,
                           const std::string &InputFileName,
                           const std::string &OutputBCFileName) {
  Context *C = NULL;
  std::string ResourceId = "";

  if (!GetClassNameFromFileName(OutputBCFileName, ResourceId))
    return false;

  if (ResourceId.empty())
    ResourceId = "<Resource ID>";

  if ((OutputPackageName == NULL) ||
      (*OutputPackageName == '\0') ||
      strcmp(OutputPackageName, "-") == 0)
    C = new Context(InputFileName, "<Package Name>", ResourceId, true);
  else
    C = new Context(InputFileName, OutputPackageName, ResourceId, false);

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
    " * Copyright (C) 2010 The Android Open Source Project\n"
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
  // RenderScript java class
  "android.renderscript.*",
  // Import R
  "android.content.res.Resources",
  // Import for debugging
  "android.util.Log",
};

const char *RSReflection::Context::AccessModifierStr(AccessModifier AM) {
  switch (AM) {
    case AM_Public: return "public"; break;
    case AM_Protected: return "protected"; break;
    case AM_Private: return "private"; break;
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

  // License
  out() << mLicenseNote;

  // Notice of generated file
  out() << "/*" << std::endl;
  out() << " * This file is auto-generated. DO NOT MODIFY!" << std::endl;
  out() << " * The source RenderScript file: " << mInputRSFile << std::endl;
  out() << " */" << std::endl;

  // Package
  if (!mPackageName.empty())
    out() << "package " << mPackageName << ";" << std::endl;
  out() << std::endl;

  // Imports
  for (unsigned i = 0;i < (sizeof(Import) / sizeof(const char*)); i++)
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
