
/* SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, bits, type) */
#ifdef SLANG_RS_SUPPORT_BUILTIN_TYPE
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Char_U, RSExportPrimitiveType::DataTypeUnsigned8)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::UChar,  RSExportPrimitiveType::DataTypeUnsigned8)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Char_S, RSExportPrimitiveType::DataTypeSigned8)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::SChar,  RSExportPrimitiveType::DataTypeSigned8)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Char16, RSExportPrimitiveType::DataTypeSigned16)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::UShort, RSExportPrimitiveType::DataTypeUnsigned16)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Short,  RSExportPrimitiveType::DataTypeSigned16)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Char32, RSExportPrimitiveType::DataTypeSigned32)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::UInt,   RSExportPrimitiveType::DataTypeUnsigned32)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Int,    RSExportPrimitiveType::DataTypeSigned32)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Float,  RSExportPrimitiveType::DataTypeFloat32)
    SLANG_RS_SUPPORT_BUILTIN_TYPE(BuiltinType::Bool,   RSExportPrimitiveType::DataTypeBool)
#   undef SLANG_RS_SUPPORT_BUILTIN_TYPE
#endif
