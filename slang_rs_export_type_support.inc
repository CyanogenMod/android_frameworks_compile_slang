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

// SLANG_RS_SUPPORT_BUILTIN_TYPE(builtin_type, type)
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
