
// SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, bits, type)
#ifdef SLANG_RS_SUPPORT_BUILTIN_TYPE
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Char_U,    RSExportPrimitiveType::DataTypeUnsigned8)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::UChar,     RSExportPrimitiveType::DataTypeUnsigned8)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Char_S,    RSExportPrimitiveType::DataTypeSigned8)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::SChar,     RSExportPrimitiveType::DataTypeSigned8)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Char16,    RSExportPrimitiveType::DataTypeSigned16)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::UShort,    RSExportPrimitiveType::DataTypeUnsigned16)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Short,     RSExportPrimitiveType::DataTypeSigned16)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Char32,    RSExportPrimitiveType::DataTypeSigned32)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::UInt,      RSExportPrimitiveType::DataTypeUnsigned32)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Int,       RSExportPrimitiveType::DataTypeSigned32)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::ULong,     RSExportPrimitiveType::DataTypeSigned64)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Long,      RSExportPrimitiveType::DataTypeSigned64)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::ULongLong, RSExportPrimitiveType::DataTypeSigned64)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::LongLong,  RSExportPrimitiveType::DataTypeSigned64)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Float,     RSExportPrimitiveType::DataTypeFloat32)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Double,    RSExportPrimitiveType::DataTypeFloat64)
  SLANG_RS_SUPPORT_BUILTIN_TYPE(clang::BuiltinType::Bool,      RSExportPrimitiveType::DataTypeBoolean)
#   undef SLANG_RS_SUPPORT_BUILTIN_TYPE
#endif
