#include "slang_rs_context.hpp"
#include "slang_rs_export_var.hpp"
#include "slang_rs_reflection.hpp"
#include "slang_rs_export_func.hpp"

#include <ctype.h>

#include <utility>
#include <cstdarg>

using std::make_pair;
using std::endl;


#define RS_SCRIPT_CLASS_NAME_PREFIX         "ScriptC_"
#define RS_SCRIPT_CLASS_SUPER_CLASS_NAME    "ScriptC"

#define RS_TYPE_CLASS_NAME_PREFIX           "ScriptField_"
#define RS_TYPE_CLASS_SUPER_CLASS_NAME      "android.renderscript.Script.FieldBase"

#define RS_TYPE_ITEM_CLASS_NAME             "Item"

#define RS_TYPE_ITEM_BUFFER_NAME            "mItemArray"
#define RS_TYPE_ITEM_BUFFER_PACKER_NAME     "mIOBuffer"

#define RS_EXPORT_VAR_INDEX_PREFIX          "mExportVarIdx_"
#define RS_EXPORT_VAR_PREFIX                "mExportVar_"

#define RS_EXPORT_FUNC_INDEX_PREFIX         "mExportFuncIdx_"

#define RS_EXPORT_VAR_ALLOCATION_PREFIX     "mAlloction_"
#define RS_EXPORT_VAR_DATA_STORAGE_PREFIX   "mData_"

namespace slang {

/* Some utility function using internal in RSReflection */
static bool GetFileNameWithoutExtension(const std::string& FileName, std::string& OutFileName) {
    OutFileName.clear();

    if(FileName.empty() || (FileName == "-"))
        return true;

    /* find last component in given path */
    size_t SlashPos = FileName.find_last_of("/\\");

    if((SlashPos != std::string::npos) && ((SlashPos + 1) < FileName.length()))
        OutFileName = FileName.substr(SlashPos + 1);
    else
        OutFileName = FileName;

    size_t DotPos = OutFileName.find_first_of('.');
    if(DotPos != std::string::npos)
        OutFileName.erase(DotPos);

    return true;
}

static const char* GetPrimitiveTypeName(const RSExportPrimitiveType* EPT) {
    static const char* PrimitiveTypeJavaNameMap[] = {
        "",
        "",
        "float",    /* RSExportPrimitiveType::DataTypeFloat32 */
        "double",   /* RSExportPrimitiveType::DataTypeFloat64 */
        "byte",     /* RSExportPrimitiveType::DataTypeSigned8 */
        "short",    /* RSExportPrimitiveType::DataTypeSigned16 */
        "int",      /* RSExportPrimitiveType::DataTypeSigned32 */
        "long",     /* RSExportPrimitiveType::DataTypeSigned64 */
        "short",    /* RSExportPrimitiveType::DataTypeUnsigned8 */
        "int",      /* RSExportPrimitiveType::DataTypeUnsigned16 */
        "long",     /* RSExportPrimitiveType::DataTypeUnsigned32 */
        "long",     /* RSExportPrimitiveType::DataTypeUnsigned64 */

        "int",      /* RSExportPrimitiveType::DataTypeUnsigned565 */
        "int",      /* RSExportPrimitiveType::DataTypeUnsigned5551 */
        "int",      /* RSExportPrimitiveType::DataTypeUnsigned4444 */

        "Element",  /* RSExportPrimitiveType::DataTypeRSElement */
        "Type",     /* RSExportPrimitiveType::DataTypeRSType */
        "Allocation",   /* RSExportPrimitiveType::DataTypeRSAllocation */
        "Sampler",  /* RSExportPrimitiveType::DataTypeRSSampler */
        "Script",   /* RSExportPrimitiveType::DataTypeRSScript */
        "SimpleMesh",       /* RSExportPrimitiveType::DataTypeRSSimpleMesh */
        "ProgramFragment",  /* RSExportPrimitiveType::DataTypeRSProgramFragment */
        "ProgramVertex",    /* RSExportPrimitiveType::DataTypeRSProgramVertex */
        "ProgramRaster",    /* RSExportPrimitiveType::DataTypeRSProgramRaster */
        "ProgramStore",     /* RSExportPrimitiveType::DataTypeRSProgramStore */
    };

    if((EPT->getType() >= 0) && (EPT->getType() < (sizeof(PrimitiveTypeJavaNameMap) / sizeof(const char*))))
        return PrimitiveTypeJavaNameMap[ EPT->getType() ];

    assert(false && "GetPrimitiveTypeName : Unknown primitive data type");
    return NULL;
}

static const char* GetVectorTypeName(const RSExportVectorType* EVT) {
    static const char* VectorTypeJavaNameMap[][3] = {
        /* 0 */ { "Byte2",  "Byte3",    "Byte4" },
        /* 1 */ { "Short2", "Short3",   "Short4" },
        /* 2 */ { "Int2",   "Int3",     "Int4" },
        /* 3 */ { "Long2",  "Long3",    "Long4" },
        /* 4 */ { "Float2", "Float3",   "Float4" },
    };

    const char** BaseElement;

    switch(EVT->getType()) {
        case RSExportPrimitiveType::DataTypeSigned8:
            BaseElement = VectorTypeJavaNameMap[0];
        break;

        case RSExportPrimitiveType::DataTypeSigned16:
        case RSExportPrimitiveType::DataTypeUnsigned8:
            BaseElement = VectorTypeJavaNameMap[1];
        break;

        case RSExportPrimitiveType::DataTypeSigned32:
        case RSExportPrimitiveType::DataTypeUnsigned16:
            BaseElement = VectorTypeJavaNameMap[2];
        break;

        case RSExportPrimitiveType::DataTypeUnsigned32:
            BaseElement = VectorTypeJavaNameMap[3];
        break;

        case RSExportPrimitiveType::DataTypeFloat32:
            BaseElement = VectorTypeJavaNameMap[4];
        break;

        default:
            assert(false && "RSReflection::genElementTypeName : Unsupported vector element data type");
        break;
    }

    assert((EVT->getNumElement() > 1) && (EVT->getNumElement() <= 4) && "Number of element in vector type is invalid");

    return BaseElement[EVT->getNumElement() - 2];
}

static const char* GetPackerAPIName(const RSExportPrimitiveType* EPT) {
    static const char* PrimitiveTypePackerAPINameMap[] = {
        "",
        "",
        "addF32",   /* RSExportPrimitiveType::DataTypeFloat32 */
        "addF64",   /* RSExportPrimitiveType::DataTypeFloat64 */
        "addI8",    /* RSExportPrimitiveType::DataTypeSigned8 */
        "addI16",   /* RSExportPrimitiveType::DataTypeSigned16 */
        "addI32",   /* RSExportPrimitiveType::DataTypeSigned32 */
        "addI64",   /* RSExportPrimitiveType::DataTypeSigned64 */
        "addU8",    /* RSExportPrimitiveType::DataTypeUnsigned8 */
        "addU16",   /* RSExportPrimitiveType::DataTypeUnsigned16 */
        "addU32",   /* RSExportPrimitiveType::DataTypeUnsigned32 */
        "addU64",   /* RSExportPrimitiveType::DataTypeUnsigned64 */

        "addU16",   /* RSExportPrimitiveType::DataTypeUnsigned565 */
        "addU16",   /* RSExportPrimitiveType::DataTypeUnsigned5551 */
        "addU16",   /* RSExportPrimitiveType::DataTypeUnsigned4444 */

        "addObj",   /* RSExportPrimitiveType::DataTypeRSElement */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSType */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSAllocation */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSSampler */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSScript */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSSimpleMesh */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSProgramFragment */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSProgramVertex */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSProgramRaster */
        "addObj",   /* RSExportPrimitiveType::DataTypeRSProgramStore */
    };

    if((EPT->getType() >= 0) && (EPT->getType() < (sizeof(PrimitiveTypePackerAPINameMap) / sizeof(const char*))))
        return PrimitiveTypePackerAPINameMap[ EPT->getType() ];

    assert(false && "GetPackerAPIName : Unknown primitive data type");
    return NULL;
}

static std::string GetTypeName(const RSExportType* ET) {
    switch(ET->getClass()) {
        case RSExportType::ExportClassPrimitive:
            return GetPrimitiveTypeName(static_cast<const RSExportPrimitiveType*>(ET));
        break;

        case RSExportType::ExportClassPointer:
        {
            const RSExportType* PointeeType = static_cast<const RSExportPointerType*>(ET)->getPointeeType();

            if(PointeeType->getClass() != RSExportType::ExportClassRecord)
                return "Allocation";
            else
                return RS_TYPE_CLASS_NAME_PREFIX + PointeeType->getName();
        }
        break;

        case RSExportType::ExportClassVector:
            return GetVectorTypeName(static_cast<const RSExportVectorType*>(ET));
        break;

        case RSExportType::ExportClassRecord:
            return RS_TYPE_CLASS_NAME_PREFIX + ET->getName() + "."RS_TYPE_ITEM_CLASS_NAME;
        break;

        default:
            assert(false && "Unknown class of type");
        break;
    }

    return "";
}

static const char* GetBuiltinElementConstruct(const RSExportType* ET) {
    if(ET->getClass() == RSExportType::ExportClassPrimitive) {
        const RSExportPrimitiveType* EPT = static_cast<const RSExportPrimitiveType*>(ET);
        if(EPT->getKind() == RSExportPrimitiveType::DataKindUser) {
            static const char* PrimitiveBuiltinElementConstructMap[] = {
                NULL,
                NULL,
                "USER_F32", /* RSExportPrimitiveType::DataTypeFloat32 */
                NULL,       /* RSExportPrimitiveType::DataTypeFloat64 */
                "USER_I8",  /* RSExportPrimitiveType::DataTypeSigned8 */
                NULL,       /* RSExportPrimitiveType::DataTypeSigned16 */
                "USER_I32", /* RSExportPrimitiveType::DataTypeSigned32 */
                NULL,       /* RSExportPrimitiveType::DataTypeSigned64 */
                "USER_U8",  /* RSExportPrimitiveType::DataTypeUnsigned8 */
                NULL,       /* RSExportPrimitiveType::DataTypeUnsigned16 */
                "USER_U32", /* RSExportPrimitiveType::DataTypeUnsigned32 */
                NULL,       /* RSExportPrimitiveType::DataTypeUnsigned64 */

                NULL,   /* RSExportPrimitiveType::DataTypeUnsigned565 */
                NULL,   /* RSExportPrimitiveType::DataTypeUnsigned5551 */
                NULL,   /* RSExportPrimitiveType::DataTypeUnsigned4444 */

                "USER_ELEMENT", /* RSExportPrimitiveType::DataTypeRSElement */
                "USER_TYPE",    /* RSExportPrimitiveType::DataTypeRSType */
                "USER_ALLOCATION",  /* RSExportPrimitiveType::DataTypeRSAllocation */
                "USER_SAMPLER",     /* RSExportPrimitiveType::DataTypeRSSampler */
                "USER_SCRIPT",      /* RSExportPrimitiveType::DataTypeRSScript */
                "USER_MESH",        /* RSExportPrimitiveType::DataTypeRSSimpleMesh */
                "USER_PROGRAM_FRAGMENT",    /* RSExportPrimitiveType::DataTypeRSProgramFragment */
                "USER_PROGRAM_VERTEX",      /* RSExportPrimitiveType::DataTypeRSProgramVertex */
                "USER_PROGRAM_RASTER",      /* RSExportPrimitiveType::DataTypeRSProgramRaster */
                "USER_PROGRAM_STORE",       /* RSExportPrimitiveType::DataTypeRSProgramStore */
            };

            if((EPT->getType() >= 0) && (EPT->getType() < (sizeof(PrimitiveBuiltinElementConstructMap) / sizeof(const char*))))
                return PrimitiveBuiltinElementConstructMap[ EPT->getType() ];
        } else if(EPT->getKind() == RSExportPrimitiveType::DataKindPixelA) {
            if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
                return "A_8";
        } else if(EPT->getKind() == RSExportPrimitiveType::DataKindPixelRGB) {
            if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned565)
                return "RGB_565";
            else if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
                return "RGB_888";
        } else if(EPT->getKind() == RSExportPrimitiveType::DataKindPixelRGBA) {
            if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned5551)
                return "RGB_5551";
            else if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned4444)
                return "RGB_4444";
            else if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned8)
                return "RGB_8888";
        } else if(EPT->getKind() == RSExportPrimitiveType::DataKindIndex) {
            if(EPT->getType() == RSExportPrimitiveType::DataTypeUnsigned16)
                return "INDEX_16";
        }
    } else if(ET->getClass() == RSExportType::ExportClassVector) {
        const RSExportVectorType* EVT = static_cast<const RSExportVectorType*>(ET);
        if(EVT->getKind() == RSExportPrimitiveType::DataKindPosition) {
            if(EVT->getType() == RSExportPrimitiveType::DataTypeFloat32) {
                if(EVT->getNumElement() == 2)
                    return "ATTRIB_POSITION_2";
                else if(EVT->getNumElement() == 3)
                    return "ATTRIB_POSITION_3";
            }
        } else if(EVT->getKind() == RSExportPrimitiveType::DataKindTexture) {
            if(EVT->getType() == RSExportPrimitiveType::DataTypeFloat32) {
                if(EVT->getNumElement() == 2)
                    return "ATTRIB_TEXTURE_2";
            }
        } else if(EVT->getKind() == RSExportPrimitiveType::DataKindNormal) {
            if(EVT->getType() == RSExportPrimitiveType::DataTypeFloat32) {
                if(EVT->getNumElement() == 3)
                    return "ATTRIB_NORMAL_3";
            }
        } else if(EVT->getKind() == RSExportPrimitiveType::DataKindColor) {
            if(EVT->getType() == RSExportPrimitiveType::DataTypeFloat32) {
                if(EVT->getNumElement() == 4)
                    return "ATTRIB_COLOR_F32_4";
            } else if(EVT->getType() == RSExportPrimitiveType::DataTypeUnsigned8) {
                if(EVT->getNumElement() == 4)
                    return "ATTRIB_COLOR_U8_4";
            }
        }
    } else if(ET->getClass() == RSExportType::ExportClassPointer) {
        return "USER_I32";  /* tread pointer type variable as unsigned int (TODO: this is target dependent) */
    }

    return NULL;
}

static const char* GetElementDataKindName(RSExportPrimitiveType::DataKind DK) {
    static const char* ElementDataKindNameMap[] = {
        "Element.DataKind.USER",        /* RSExportPrimitiveType::DataKindUser */
        "Element.DataKind.COLOR",       /* RSExportPrimitiveType::DataKindColor */
        "Element.DataKind.POSITION",    /* RSExportPrimitiveType::DataKindPosition */
        "Element.DataKind.TEXTURE",     /* RSExportPrimitiveType::DataKindTexture */
        "Element.DataKind.NORMAL",      /* RSExportPrimitiveType::DataKindNormal */
        "Element.DataKind.INDEX",       /* RSExportPrimitiveType::DataKindIndex */
        "Element.DataKind.POINT_SIZE",  /* RSExportPrimitiveType::DataKindPointSize */
        "Element.DataKind.PIXEL_L",     /* RSExportPrimitiveType::DataKindPixelL */
        "Element.DataKind.PIXEL_A",     /* RSExportPrimitiveType::DataKindPixelA */
        "Element.DataKind.PIXEL_LA",    /* RSExportPrimitiveType::DataKindPixelLA */
        "Element.DataKind.PIXEL_RGB",   /* RSExportPrimitiveType::DataKindPixelRGB */
        "Element.DataKind.PIXEL_RGBA",  /* RSExportPrimitiveType::DataKindPixelRGBA */
    };

    if((DK >= 0) && (DK < (sizeof(ElementDataKindNameMap) / sizeof(const char*))))
        return ElementDataKindNameMap[ DK ];
    else
        return NULL;
}

static const char* GetElementDataTypeName(RSExportPrimitiveType::DataType DT) {
    static const char* ElementDataTypeNameMap[] = {
        NULL,
        NULL,
        "Element.DataType.FLOAT_32",    /* RSExportPrimitiveType::DataTypeFloat32 */
        NULL,                           /* RSExportPrimitiveType::DataTypeFloat64 */
        "Element.DataType.SIGNED_8",    /* RSExportPrimitiveType::DataTypeSigned8 */
        "Element.DataType.SIGNED_16",   /* RSExportPrimitiveType::DataTypeSigned16 */
        "Element.DataType.SIGNED_32",   /* RSExportPrimitiveType::DataTypeSigned32 */
        NULL,                           /* RSExportPrimitiveType::DataTypeSigned64 */
        "Element.DataType.UNSIGNED_8",  /* RSExportPrimitiveType::DataTypeUnsigned8 */
        "Element.DataType.UNSIGNED_16", /* RSExportPrimitiveType::DataTypeUnsigned16 */
        "Element.DataType.UNSIGNED_32", /* RSExportPrimitiveType::DataTypeUnsigned32 */
        NULL,                           /* RSExportPrimitiveType::DataTypeUnsigned64 */

        "Element.DataType.UNSIGNED_5_6_5",   /* RSExportPrimitiveType::DataTypeUnsigned565 */
        "Element.DataType.UNSIGNED_5_5_5_1", /* RSExportPrimitiveType::DataTypeUnsigned5551 */
        "Element.DataType.UNSIGNED_4_4_4_4", /* RSExportPrimitiveType::DataTypeUnsigned4444 */

        "Element.DataType.RS_ELEMENT",  /* RSExportPrimitiveType::DataTypeRSElement */
        "Element.DataType.RS_TYPE",     /* RSExportPrimitiveType::DataTypeRSType */
        "Element.DataType.RS_ALLOCATION",   /* RSExportPrimitiveType::DataTypeRSAllocation */
        "Element.DataType.RS_SAMPLER",      /* RSExportPrimitiveType::DataTypeRSSampler */
        "Element.DataType.RS_SCRIPT",       /* RSExportPrimitiveType::DataTypeRSScript */
        "Element.DataType.RS_MESH",         /* RSExportPrimitiveType::DataTypeRSSimpleMesh */
        "Element.DataType.RS_PROGRAM_FRAGMENT", /* RSExportPrimitiveType::DataTypeRSProgramFragment */
        "Element.DataType.RS_PROGRAM_VERTEX",   /* RSExportPrimitiveType::DataTypeRSProgramVertex */
        "Element.DataType.RS_PROGRAM_RASTER",   /* RSExportPrimitiveType::DataTypeRSProgramRaster */
        "Element.DataType.RS_PROGRAM_STORE",    /* RSExportPrimitiveType::DataTypeRSProgramStore */
    };

    if((DT >= 0) && (DT < (sizeof(ElementDataTypeNameMap) / sizeof(const char*))))
        return ElementDataTypeNameMap[ DT ];
    else
        return NULL;
}

/****************************** Methods to generate script class ******************************/
bool RSReflection::genScriptClass(Context& C, const std::string& ClassName, std::string& ErrorMsg) {
    if(!C.startClass(Context::AM_Public, false, ClassName, RS_SCRIPT_CLASS_SUPER_CLASS_NAME, ErrorMsg))
        return false;

    genScriptClassConstructor(C);

    /* Reflect export variable */
    for(RSContext::const_export_var_iterator I = mRSContext->export_vars_begin();
        I != mRSContext->export_vars_end();
        I++)
        genExportVariable(C, *I);

    /* Reflect export function */
    for(RSContext::const_export_func_iterator I = mRSContext->export_funcs_begin();
        I != mRSContext->export_funcs_end();
        I++)
        genExportFunction(C, *I);

    C.endClass();

    return true;
}

void RSReflection::genScriptClassConstructor(Context& C) {
    C.indent() << "// Constructor" << endl;
    C.startFunction(Context::AM_Public, false, NULL, C.getClassName(), 4, "RenderScript", "rs",
                                                                          "Resources", "resources",
                                                                          "int", "id",
                                                                          "boolean", "isRoot");
    /* Call constructor of super class */
    C.indent() << "super(rs, resources, id, isRoot);" << endl;
    C.endFunction();
    return;
}

void RSReflection::genExportVariable(Context& C, const RSExportVar* EV) {
    const RSExportType* ET = EV->getType();

    C.indent() << "private final static int "RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << " = " << C.getNextExportVarSlot() << ";" << endl;

    switch(ET->getClass()) {
        case RSExportType::ExportClassPrimitive:
            genPrimitiveTypeExportVariable(C, EV);
        break;

        case RSExportType::ExportClassPointer:
            genPointerTypeExportVariable(C, EV);
        break;

        case RSExportType::ExportClassVector:
            genVectorTypeExportVariable(C, EV);
        break;

        case RSExportType::ExportClassRecord:
            genRecordTypeExportVariable(C, EV);
        break;

        default:
            assert(false && "Unknown class of type");
        break;
    }

    return;
}

void RSReflection::genExportFunction(Context& C, const RSExportFunc* EF) {
    C.indent() << "private final static int "RS_EXPORT_FUNC_INDEX_PREFIX << EF->getName() << " = " << C.getNextExportFuncSlot() << ";" << endl;

    /* invoke_*() */
    Context::ArgTy Args;

    for(RSExportFunc::const_param_iterator I = EF->params_begin();
        I != EF->params_end();
        I++)
    {
        const RSExportFunc::Parameter* P = *I;
        Args.push_back( make_pair(GetTypeName(P->getType()), P->getName()) );
    }

    C.startFunction(Context::AM_Public, false, "void", "invoke_" + EF->getName(), Args);

    if(!EF->hasParam()) {
        C.indent() << "invoke("RS_EXPORT_FUNC_INDEX_PREFIX << EF->getName() << ");" << endl;
    } else {
        const RSExportRecordType* ERT = EF->getParamPacketType();
        std::string FieldPackerName = EF->getName() + "_fp";

        if(genCreateFieldPacker(C, ERT, FieldPackerName.c_str()))
            genPackVarOfType(C, ERT, NULL, FieldPackerName.c_str());

        C.indent() << "invoke("RS_EXPORT_FUNC_INDEX_PREFIX << EF->getName() << ", " << FieldPackerName << ");" << endl;
    }

    C.endFunction();
    return;
}

void RSReflection::genPrimitiveTypeExportVariable(Context& C, const RSExportVar* EV) {
    assert((EV->getType()->getClass() == RSExportType::ExportClassPrimitive) && "Variable should be type of primitive here");

    const RSExportPrimitiveType* EPT = static_cast<const RSExportPrimitiveType*>(EV->getType());
    const char* TypeName = GetPrimitiveTypeName(EPT);

    C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX << EV->getName() << ";" << endl;

    /* set_*() */
    C.startFunction(Context::AM_Public, false, "void", "set_" + EV->getName(), 1, TypeName, "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << endl;

    if(EPT->isRSObjectType())
        C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", v.getID());" << endl;
    else
        C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", v);" << endl;

    C.endFunction();

    genGetExportVariable(C, TypeName, EV->getName());

    return;
}

void RSReflection::genPointerTypeExportVariable(Context& C, const RSExportVar* EV) {
    const RSExportType* ET = EV->getType();
    const RSExportType* PointeeType;
    std::string TypeName;

    assert((ET->getClass() == RSExportType::ExportClassPointer) && "Variable should be type of pointer here");

    PointeeType = static_cast<const RSExportPointerType*>(ET)->getPointeeType();
    TypeName = GetTypeName(ET);

    /* bind_*() */
    C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX << EV->getName() << ";" << endl;

    C.startFunction(Context::AM_Public, false, "void", "bind_" + EV->getName(), 1, TypeName.c_str(), "v");

    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << endl;
    C.indent() << "if(v == null) bindAllocation(null, "RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ");" << endl;

    if(PointeeType->getClass() == RSExportType::ExportClassRecord)
        C.indent() << "else bindAllocation(v.getAllocation(), "RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ");" << endl;
    else
        C.indent() << "else bindAllocation(v, "RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ");" << endl;

    C.endFunction();

    genGetExportVariable(C, TypeName, EV->getName());

    return;
}

void RSReflection::genVectorTypeExportVariable(Context& C, const RSExportVar* EV) {
    assert((EV->getType()->getClass() == RSExportType::ExportClassVector) && "Variable should be type of vector here");

    const RSExportVectorType* EVT = static_cast<const RSExportVectorType*>(EV->getType());
    const char* TypeName = GetVectorTypeName(EVT);
    const char* FieldPackerName = "fp";

    C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX << EV->getName() << ";" << endl;

    /* set_*() */
    C.startFunction(Context::AM_Public, false, "void", "set_" + EV->getName(), 1, TypeName, "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << endl;

    if(genCreateFieldPacker(C, EVT, FieldPackerName))
        genPackVarOfType(C, EVT, "v", FieldPackerName);
    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", " << FieldPackerName << ");" << endl;

    C.endFunction();

    genGetExportVariable(C, TypeName, EV->getName());
    return;
}

void RSReflection::genRecordTypeExportVariable(Context& C, const RSExportVar* EV) {
    assert((EV->getType()->getClass() == RSExportType::ExportClassRecord) && "Variable should be type of struct here");

    const RSExportRecordType* ERT = static_cast<const RSExportRecordType*>(EV->getType());
    std::string TypeName = RS_TYPE_CLASS_NAME_PREFIX + ERT->getName() + "."RS_TYPE_ITEM_CLASS_NAME;
    const char* FieldPackerName = "fp";

    C.indent() << "private " << TypeName << " "RS_EXPORT_VAR_PREFIX << EV->getName() << ";" << endl;

    /* set_*() */
    C.startFunction(Context::AM_Public, false, "void", "set_" + EV->getName(), 1, TypeName.c_str(), "v");
    C.indent() << RS_EXPORT_VAR_PREFIX << EV->getName() << " = v;" << endl;

    if(genCreateFieldPacker(C, ERT, FieldPackerName))
        genPackVarOfType(C, ERT, "v", FieldPackerName);
    C.indent() << "setVar("RS_EXPORT_VAR_INDEX_PREFIX << EV->getName() << ", " << FieldPackerName << ");" << endl;

    C.endFunction();

    genGetExportVariable(C, TypeName.c_str(), EV->getName());
    return;
}

void RSReflection::genGetExportVariable(Context& C, const std::string& TypeName, const std::string& VarName) {
    C.startFunction(Context::AM_Public, false, TypeName.c_str(), "get_" + VarName, 0);

    C.indent() << "return "RS_EXPORT_VAR_PREFIX << VarName << ";" << endl;

    C.endFunction();
    return;
}

/****************************** Methods to generate script class /end ******************************/

bool RSReflection::genCreateFieldPacker(Context& C, const RSExportType* ET, const char* FieldPackerName) {
    size_t AllocSize = RSExportType::GetTypeAllocSize(ET);
    if(AllocSize > 0)
        C.indent() << "FieldPacker " << FieldPackerName << " = new FieldPacker(" << AllocSize << ");" << endl;
    else
        return false;
    return true;
}

void RSReflection::genPackVarOfType(Context& C, const RSExportType* ET, const char* VarName, const char* FieldPackerName) {
    switch(ET->getClass()) {
        case RSExportType::ExportClassPrimitive:
        case RSExportType::ExportClassVector:
            C.indent() << FieldPackerName << "." << GetPackerAPIName(static_cast<const RSExportPrimitiveType*>(ET)) << "(" << VarName << ");" << endl;
        break;

        case RSExportType::ExportClassPointer:
        {
            /* Must reflect as type Allocation in Java */
            const RSExportType* PointeeType = static_cast<const RSExportPointerType*>(ET)->getPointeeType();

            if(PointeeType->getClass() != RSExportType::ExportClassRecord)
                C.indent() << FieldPackerName << ".addI32(" << VarName << ".getPtr());" << endl;
            else
                C.indent() << FieldPackerName << ".addI32(" << VarName << ".getAllocation().getPtr());" << endl;
        }
        break;

        case RSExportType::ExportClassRecord:
        {
            const RSExportRecordType* ERT = static_cast<const RSExportRecordType*>(ET);
            int Pos = 0;    /* relative pos from now on in field packer */

            for(RSExportRecordType::const_field_iterator I = ERT->fields_begin();
                I != ERT->fields_end();
                I++)
            {
                const RSExportRecordType::Field* F = *I;
                std::string FieldName;
                size_t FieldOffset = F->getOffsetInParent();
                size_t FieldStoreSize = RSExportType::GetTypeStoreSize(F->getType());
                size_t FieldAllocSize = RSExportType::GetTypeAllocSize(F->getType());

                if(VarName != NULL)
                    FieldName = VarName + ("." + F->getName());
                else
                    FieldName = F->getName();

                if(FieldOffset > Pos)
                    C.indent() << FieldPackerName << ".skip(" << (FieldOffset - Pos) << ");" << endl;

                genPackVarOfType(C, F->getType(), FieldName.c_str(), FieldPackerName);

                if(FieldAllocSize > FieldStoreSize)  /* there's padding in the field type */
                    C.indent() << FieldPackerName << ".skip(" << (FieldAllocSize - FieldStoreSize) << ");" << endl;

                Pos = FieldOffset + FieldAllocSize;
            }

            /* There maybe some padding after the struct */
            size_t Padding = RSExportType::GetTypeAllocSize(ERT) - Pos;
            if(Padding > 0)
                C.indent() << FieldPackerName << ".skip(" << Padding << ");" << endl;
        }
        break;

        default:
            assert(false && "Unknown class of type");
        break;
    }

    return;
}

/****************************** Methods to generate type class  ******************************/
bool RSReflection::genTypeClass(Context& C, const RSExportRecordType* ERT, std::string& ErrorMsg) {
    std::string ClassName = RS_TYPE_CLASS_NAME_PREFIX + ERT->getName();

    if(!C.startClass(Context::AM_Public, false, ClassName, RS_TYPE_CLASS_SUPER_CLASS_NAME, ErrorMsg))
        return false;

    if(!genTypeItemClass(C, ERT, ErrorMsg))
        return false;

    /* Declare item buffer and item buffer packer */
    C.indent() << "private "RS_TYPE_ITEM_CLASS_NAME" "RS_TYPE_ITEM_BUFFER_NAME"[];" << endl;
    C.indent() << "private FieldPacker "RS_TYPE_ITEM_BUFFER_PACKER_NAME";" << endl;

    genTypeClassConstructor(C, ERT);
    genTypeClassCopyToArray(C, ERT);
    genTypeClasSet(C, ERT);
    genTypeClasCopyAll(C, ERT);

    C.endClass();

    return true;
}

bool RSReflection::genTypeItemClass(Context& C, const RSExportRecordType* ERT, std::string& ErrorMsg) {
    C.indent() << "static public class "RS_TYPE_ITEM_CLASS_NAME;
    C.startBlock();

    C.indent() << "public static final int sizeof = " << RSExportType::GetTypeAllocSize(ERT) << ";" << endl;

    /* Member elements */
    C.out() << endl;
    for(RSExportRecordType::const_field_iterator FI = ERT->fields_begin();
        FI != ERT->fields_end();
        FI++)
        C.indent() << GetTypeName((*FI)->getType()) << " " << (*FI)->getName() << ";" << endl;

    /* Constructor */
    C.out() << endl;
    C.indent() << RS_TYPE_ITEM_CLASS_NAME"()";
    C.startBlock();

    for(RSExportRecordType::const_field_iterator FI = ERT->fields_begin();
        FI != ERT->fields_end();
        FI++)
    {
        const RSExportRecordType::Field* F = *FI;
        if((F->getType()->getClass() == RSExportType::ExportClassVector) || (F->getType()->getClass() == RSExportType::ExportClassRecord))
        C.indent() << F->getName() << " = new " << GetTypeName(F->getType()) << "();" << endl;
    }

    C.endBlock();   /* end Constructor */

    C.endBlock();   /* end Item class */


    return true;
}

void RSReflection::genTypeClassConstructor(Context& C, const RSExportRecordType* ERT) {
    const char* RenderScriptVar = "rs";

    C.startFunction(Context::AM_Public, false, NULL, C.getClassName(), 2, "RenderScript", RenderScriptVar,
                                                                          "int", "count");

    C.indent() << RS_TYPE_ITEM_BUFFER_NAME" = null;" << endl;
    C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME" = null;" << endl;

    genBuildElement(C, ERT, "mElement", RenderScriptVar);
    /* Call init() in super class */
    C.indent() << "init(" << RenderScriptVar << ", count);" << endl;
    C.endFunction();

    return;
}

void RSReflection::genTypeClassCopyToArray(Context& C, const RSExportRecordType* ERT) {
    C.startFunction(Context::AM_Private, false, "void", "copyToArray", 2, RS_TYPE_ITEM_CLASS_NAME, "i",
                                                                          "int", "index");

    C.indent() << "if ("RS_TYPE_ITEM_BUFFER_PACKER_NAME" == null) "RS_TYPE_ITEM_BUFFER_PACKER_NAME" = new FieldPacker("RS_TYPE_ITEM_CLASS_NAME".sizeof * mType.getX() /* count */);" << endl;
    C.indent() << RS_TYPE_ITEM_BUFFER_PACKER_NAME".reset(index * "RS_TYPE_ITEM_CLASS_NAME".sizeof);" << endl;

    genPackVarOfType(C, ERT, "i", RS_TYPE_ITEM_BUFFER_PACKER_NAME);

    C.endFunction();
    return;
}

void RSReflection::genTypeClasSet(Context& C, const RSExportRecordType* ERT) {
    C.startFunction(Context::AM_Public, false, "void", "set", 3, RS_TYPE_ITEM_CLASS_NAME, "i",
                                                                 "int", "index",
                                                                 "boolean", "copyNow");
    C.indent() << "if ("RS_TYPE_ITEM_BUFFER_NAME" == null) "RS_TYPE_ITEM_BUFFER_NAME" = new "RS_TYPE_ITEM_CLASS_NAME"[mType.getX() /* count */];" << endl;
    C.indent() << RS_TYPE_ITEM_BUFFER_NAME"[index] = i;" << endl;

    C.indent() << "if (copyNow) ";
    C.startBlock();

    C.indent() << "copyToArray(i, index);" << endl;
    C.indent() << "mAllocation.subData1D(index * "RS_TYPE_ITEM_CLASS_NAME".sizeof, "RS_TYPE_ITEM_CLASS_NAME".sizeof, "RS_TYPE_ITEM_BUFFER_PACKER_NAME".getData());" << endl;

    C.endBlock();   /* end if (copyNow) */

    C.endFunction();
    return;
}

void RSReflection::genTypeClasCopyAll(Context& C, const RSExportRecordType* ERT) {
    C.startFunction(Context::AM_Public, false, "void", "copyAll", 0);

    C.indent() << "for (int ct=0; ct < "RS_TYPE_ITEM_BUFFER_NAME".length; ct++) copyToArray("RS_TYPE_ITEM_BUFFER_NAME"[ct], ct);" << endl;
    C.indent() << "mAllocation.data("RS_TYPE_ITEM_BUFFER_PACKER_NAME".getData());" << endl;

    C.endFunction();
    return;
}

/****************************** Methods to generate type class /end ******************************/

/******************** Methods to create Element in Java of given record type ********************/
void RSReflection::genBuildElement(Context& C, const RSExportRecordType* ERT, const char* ElementName, const char* RenderScriptVar) {
    const char* ElementBuilderName = "eb";

    /* Create element builder */
    C.startBlock(true);

    C.indent() << "Element.Builder " << ElementBuilderName << " = new Element.Builder(" << RenderScriptVar << ");" << endl;

    /* eb.add(...) */
    genAddElementToElementBuilder(C, ERT, "", ElementBuilderName, RenderScriptVar);

    C.indent() << ElementName << " = " << ElementBuilderName << ".create();" << endl;

    C.endBlock();
    return;
}

#define EB_ADD(x, ...)  \
        C.indent() << ElementBuilderName << ".add(Element." << x ##__VA_ARGS__ ", \"" << VarName << "\");" << endl
void RSReflection::genAddElementToElementBuilder(Context& C, const RSExportType* ET, const std::string& VarName, const char* ElementBuilderName, const char* RenderScriptVar) {
        const char* ElementConstruct = GetBuiltinElementConstruct(ET);
        if(ElementConstruct != NULL) {
            EB_ADD(ElementConstruct << "(" << RenderScriptVar << ")");
        } else {
            if((ET->getClass() == RSExportType::ExportClassPrimitive) || (ET->getClass() == RSExportType::ExportClassVector)) {
                const RSExportPrimitiveType* EPT = static_cast<const RSExportPrimitiveType*>(ET);
                const char* DataKindName = GetElementDataKindName(EPT->getKind());
                const char* DataTypeName = GetElementDataTypeName(EPT->getType());
                int Size = (ET->getClass() == RSExportType::ExportClassVector) ? static_cast<const RSExportVectorType*>(ET)->getNumElement() : 1;

                switch(EPT->getKind()) {
                    case RSExportPrimitiveType::DataKindColor:
                    case RSExportPrimitiveType::DataKindPosition:
                    case RSExportPrimitiveType::DataKindTexture:
                    case RSExportPrimitiveType::DataKindNormal:
                    case RSExportPrimitiveType::DataKindPointSize:
                        /* Element.createAttrib() */
                        EB_ADD("createAttrib(" << RenderScriptVar << ", " << DataTypeName << ", " << DataKindName << ", " << Size << ")");
                    break;

                    case RSExportPrimitiveType::DataKindIndex:
                        /* Element.createIndex() */
                        EB_ADD("createAttrib(" << RenderScriptVar << ")");
                    break;

                    case RSExportPrimitiveType::DataKindPixelL:
                    case RSExportPrimitiveType::DataKindPixelA:
                    case RSExportPrimitiveType::DataKindPixelLA:
                    case RSExportPrimitiveType::DataKindPixelRGB:
                    case RSExportPrimitiveType::DataKindPixelRGBA:
                        /* Element.createPixel() */
                        EB_ADD("createVector(" << RenderScriptVar << ", " << DataTypeName << ", " << DataKindName << ")");
                    break;

                    case RSExportPrimitiveType::DataKindUser:
                    default:
                        if(EPT->getClass() == RSExportType::ExportClassPrimitive)
                            /* Element.createUser() */
                            EB_ADD("createUser(" << RenderScriptVar << ", " << DataTypeName << ")");
                        else /* (ET->getClass() == RSExportType::ExportClassVector) must hold here */
                            /* Element.createVector() */
                            EB_ADD("createVector(" << RenderScriptVar << ", " << DataTypeName << ", " << Size << ")");
                    break;
                }
            } else if(ET->getClass() == RSExportType::ExportClassPointer) {
                /* Pointer type variable should be resolved in GetBuiltinElementConstruct()  */
                assert(false && "??");
            } else if(ET->getClass() == RSExportType::ExportClassRecord) {
                /*
                 * Simalar to genPackVarOfType.
                 *
                 * TODO: Generalize these two function such that there's no duplicated codes.
                 */
                const RSExportRecordType* ERT = static_cast<const RSExportRecordType*>(ET);
                int Pos = 0;    /* relative pos from now on */

                for(RSExportRecordType::const_field_iterator I = ERT->fields_begin();
                    I != ERT->fields_end();
                    I++)
                {
                    const RSExportRecordType::Field* F = *I;
                    std::string FieldName;
                    size_t FieldOffset = F->getOffsetInParent();
                    size_t FieldStoreSize = RSExportType::GetTypeStoreSize(F->getType());
                    size_t FieldAllocSize = RSExportType::GetTypeAllocSize(F->getType());

                    if(!VarName.empty())
                        FieldName = VarName + "." + F->getName();
                    else
                        FieldName = F->getName();

                    /* alignment */
                    genAddPaddingToElementBuiler(C, (FieldOffset - Pos), ElementBuilderName, RenderScriptVar);

                    /* eb.add(...) */
                    genAddElementToElementBuilder(C, (*I)->getType(), FieldName, ElementBuilderName, RenderScriptVar);

                    /* there's padding within the field type */
                    genAddPaddingToElementBuiler(C, (FieldAllocSize - FieldStoreSize), ElementBuilderName, RenderScriptVar);

                    Pos = FieldOffset + FieldAllocSize;
                }

                /* There maybe some padding after the struct */
                genAddPaddingToElementBuiler(C, (RSExportType::GetTypeAllocSize(ERT) - Pos), ElementBuilderName, RenderScriptVar);
            } else {
                assert(false && "Unknown class of type");
            }
        }
}

void RSReflection::genAddPaddingToElementBuiler(Context& C, size_t PaddingSize, const char* ElementBuilderName, const char* RenderScriptVar) {
    while(PaddingSize > 0) {
        const std::string& VarName = C.createPaddingField();
        if(PaddingSize >= 4) {
            EB_ADD("USER_U32(" << RenderScriptVar << ")");
            PaddingSize -= 4;
        } else if(PaddingSize >= 2) {
            EB_ADD("USER_U16(" << RenderScriptVar << ")");
            PaddingSize -= 2;
        } else if(PaddingSize >= 1) {
            EB_ADD("USER_U8(" << RenderScriptVar << ")");
            PaddingSize -= 1;
        }
    }
    return;
}

#undef EB_ADD
/******************** Methods to create Element in Java of given record type /end ********************/

bool RSReflection::reflect(const char* OutputPackageName, const std::string& InputFileName, const std::string& OutputBCFileName) {
    Context *C = NULL;
    std::string ResourceId = "";

    if(!GetFileNameWithoutExtension(OutputBCFileName, ResourceId))
        return false;

    if(ResourceId.empty())
        ResourceId = "<Resource ID>";

    if((OutputPackageName == NULL) || (*OutputPackageName == '\0') || strcmp(OutputPackageName, "-") == 0)
        C = new Context("<Package Name>", ResourceId, true);
    else
        C = new Context(OutputPackageName, ResourceId, false);

    if(C != NULL) {
        std::string ErrorMsg, ScriptClassName;
        /* class ScriptC_<ScriptName> */
        if(!GetFileNameWithoutExtension(InputFileName, ScriptClassName))
            return false;

        if(ScriptClassName.empty())
            ScriptClassName = "<Input Script Name>";

        ScriptClassName.at(0) = toupper(ScriptClassName.at(0));
        ScriptClassName.insert(0, RS_SCRIPT_CLASS_NAME_PREFIX);

        if(!genScriptClass(*C, ScriptClassName, ErrorMsg)) {
            std::cerr << "Failed to generate class " << ScriptClassName << " (" << ErrorMsg << ")" << endl;
            return false;
        }

        /* class ScriptField_<TypeName> */
        for(RSContext::const_export_type_iterator TI = mRSContext->export_types_begin();
            TI != mRSContext->export_types_end();
            TI++)
        {
            const RSExportType* ET = TI->getValue();

            if(ET->getClass() == RSExportType::ExportClassRecord) {
                const RSExportRecordType* ERT = static_cast<const RSExportRecordType*>(ET);

                if(!ERT->isArtificial() && !genTypeClass(*C, ERT, ErrorMsg)) {
                    std::cerr << "Failed to generate type class for struct '" << ERT->getName() << "' (" << ErrorMsg << ")" << endl;
                    return false;
                }
            }
        }
    }

    return true;
}

/****************************** RSReflection::Context ******************************/
const char* const RSReflection::Context::LicenseNote =
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
	" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
	" * See the License for the specific language governing permissions and\n"
	" * limitations under the License.\n"
	" */\n";

const char* const RSReflection::Context::Import[] = {
    /* RenderScript java class */
    "android.renderscript.*",
    /* Import R */
    "android.content.res.Resources",
    /* Import for debugging */
    "android.util.Log",
};

const char* RSReflection::Context::AccessModifierStr(AccessModifier AM) {
    switch(AM) {
        case AM_Public: return "public"; break;
        case AM_Protected: return "protected"; break;
        case AM_Private: return "private"; break;
        default: return ""; break;
    }
}

bool RSReflection::Context::startClass(AccessModifier AM, bool IsStatic, const std::string& ClassName, const char* SuperClassName, std::string& ErrorMsg) {
    if(mVerbose)
        std::cout << "Generating " << ClassName << ".java ..." << endl;

    /* Open the file */
    if(!mUseStdout) {
        mOF.clear();
        mOF.open((ClassName + ".java").c_str());
        if(!mOF.good()) {
            ErrorMsg = "failed to open file '" + ClassName + ".java' for write";
            return false;
        }
    }

    /* License */
    out() << LicenseNote << endl;

    /* Package */
    if(!mPackageName.empty())
        out() << "package " << mPackageName << ";" << endl;
    out() << endl;

    /* Imports */
    for(int i=0;i<(sizeof(Import)/sizeof(const char*));i++)
        out() << "import " << Import[i] << ";" << endl;
    out() << endl;

    out() << AccessModifierStr(AM) << ((IsStatic) ? " static" : "") << " class " << ClassName;
    if(SuperClassName != NULL)
        out() << " extends " << SuperClassName;

    startBlock();

    mClassName = ClassName;

    return true;
}

void RSReflection::Context::endClass() {
    endBlock();
    if(!mUseStdout)
        mOF.close();
    clear();
    return;
}

void RSReflection::Context::startBlock(bool ShouldIndent) {
    if(ShouldIndent)
        indent() << "{" << endl;
    else
        out() << " {" << endl;
    incIndentLevel();
    return;
}

void RSReflection::Context::endBlock() {
    decIndentLevel();
    indent() << "}" << endl << endl;
    return;
}

void RSReflection::Context::startTypeClass(const std::string& ClassName) {
    indent() << "public static class " << ClassName;
    startBlock();
    return;
}

void RSReflection::Context::endTypeClass() {
    endBlock();
    return;
}

void RSReflection::Context::startFunction(AccessModifier AM, bool IsStatic, const char* ReturnType, const std::string& FunctionName, int Argc, ...) {
    ArgTy Args;
    va_list vl;
    va_start(vl, Argc);

    for(int i=0;i<Argc;i++) {
        const char* ArgType = va_arg(vl, const char*);
        const char* ArgName = va_arg(vl, const char*);

        Args.push_back( make_pair(ArgType, ArgName) );
    }
    va_end(vl);

    startFunction(AM, IsStatic, ReturnType, FunctionName, Args);

    return;
}

void RSReflection::Context::startFunction(AccessModifier AM,
                                          bool IsStatic,
                                          const char* ReturnType,
                                          const std::string& FunctionName,
                                          const ArgTy& Args)
{
    indent() << AccessModifierStr(AM) << ((IsStatic) ? " static " : " ") << ((ReturnType) ? ReturnType : "") << " " << FunctionName << "(";

    bool FirstArg = true;
    for(ArgTy::const_iterator I = Args.begin();
        I != Args.end();
        I++)
    {
        if(!FirstArg)
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

}   /* namespace slang */
