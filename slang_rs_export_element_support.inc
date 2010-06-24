#ifdef USE_ELEMENT_DATA_TYPE

#   define GET_ELEMENT_DATA_TYPE(type)      ELEMENT_DATA_TYPE_ ## type

#   define ELEMENT_DATA_TYPE_FLOAT_32       RSExportPrimitiveType::DataTypeFloat32
#   define ELEMENT_DATA_TYPE_SIGNED_8       RSExportPrimitiveType::DataTypeSigned8
#   define ELEMENT_DATA_TYPE_SIGNED_16      RSExportPrimitiveType::DataTypeSigned16
#   define ELEMENT_DATA_TYPE_SIGNED_32      RSExportPrimitiveType::DataTypeSigned32
#   define ELEMENT_DATA_TYPE_UNSIGNED_8     RSExportPrimitiveType::DataTypeUnsigned8
#   define ELEMENT_DATA_TYPE_UNSIGNED_16    RSExportPrimitiveType::DataTypeUnsigned16
#   define ELEMENT_DATA_TYPE_UNSIGNED_32    RSExportPrimitiveType::DataTypeUnsigned32
#   define ELEMENT_DATA_TYPE_UNSIGNED_565   RSExportPrimitiveType::DataTypeUnsigned565
#   define ELEMENT_DATA_TYPE_UNSIGNED_5551  RSExportPrimitiveType::DataTypeUnsigned5551
#   define ELEMENT_DATA_TYPE_UNSIGNED_4444  RSExportPrimitiveType::DataTypeUnsigned4444
#   define ELEMENT_DATA_TYPE_RS_ELEMENT     RSExportPrimitiveType::DataTypeRSElement
#   define ELEMENT_DATA_TYPE_RS_TYPE        RSExportPrimitiveType::DataTypeRSType
#   define ELEMENT_DATA_TYPE_RS_ALLOCATION  RSExportPrimitiveType::DataTypeRSAllocation
#   define ELEMENT_DATA_TYPE_RS_SAMPLER     RSExportPrimitiveType::DataTypeRSSampler
#   define ELEMENT_DATA_TYPE_RS_SCRIPT      RSExportPrimitiveType::DataTypeRSScript
#   define ELEMENT_DATA_TYPE_RS_MESH        RSExportPrimitiveType::DataTypeRSMesh
#   define ELEMENT_DATA_TYPE_RS_PROGRAM_FRAGMENT    RSExportPrimitiveType::DataTypeRSProgramFragment
#   define ELEMENT_DATA_TYPE_RS_PROGRAM_VERTEX      RSExportPrimitiveType::DataTypeRSProgramVertex
#   define ELEMENT_DATA_TYPE_RS_PROGRAM_RASTER      RSExportPrimitiveType::DataTypeRSProgramRaster
#   define ELEMENT_DATA_TYPE_RS_PROGRAM_STORE       RSExportPrimitiveType::DataTypeRSProgramStore
#   define ELEMENT_DATA_TYPE_RS_FONT                RSExportPrimitiveType::DataTypeRSFont

#endif  /* USE_ELEMENT_TYPE_NAME */

#ifdef USE_ELEMENT_DATA_KIND

#   define GET_ELEMENT_DATA_KIND(kind)  ELEMENT_DATA_KIND_ ## kind

#   define ELEMENT_DATA_KIND_USER       RSExportPrimitiveType::DataKindUser
#   define ELEMENT_DATA_KIND_COLOR      RSExportPrimitiveType::DataKindColor
#   define ELEMENT_DATA_KIND_POSITION   RSExportPrimitiveType::DataKindPosition
#   define ELEMENT_DATA_KIND_TEXTURE    RSExportPrimitiveType::DataKindTexture
#   define ELEMENT_DATA_KIND_NORMAL     RSExportPrimitiveType::DataKindNormal
#   define ELEMENT_DATA_KIND_INDEX      RSExportPrimitiveType::DataKindIndex
#   define ELEMENT_DATA_KIND_POINT_SIZE RSExportPrimitiveType::DataKindPointSize
#   define ELEMENT_DATA_KIND_PIXEL_L    RSExportPrimitiveType::DataKindPixelL
#   define ELEMENT_DATA_KIND_PIXEL_A    RSExportPrimitiveType::DataKindPixelA
#   define ELEMENT_DATA_KIND_PIXEL_LA   RSExportPrimitiveType::DataKindPixelLA
#   define ELEMENT_DATA_KIND_PIXEL_RGB  RSExportPrimitiveType::DataKindPixelRGB
#   define ELEMENT_DATA_KIND_PIXEL_RGBA RSExportPrimitiveType::DataKindPixelRGBA

#endif  /* USE_ELEMENT_TYPE_KIND */

/* DEF_ELEMENT(name, dk, dt, norm, vsize) */
#ifdef DEF_ELEMENT

    /* RS_KIND_INDEX */
    DEF_ELEMENT("rs_index", INDEX, UNSIGNED_16, false, 1)

    /* RS_KIND_COLOR */
    DEF_ELEMENT("rs_color3f", COLOR, FLOAT_32, false, 3)
    DEF_ELEMENT("rs_color4f", COLOR, FLOAT_32, false, 4)
    DEF_ELEMENT("rs_color3u", COLOR, UNSIGNED_8, true, 3)
    DEF_ELEMENT("rs_color4u", COLOR, UNSIGNED_8, true, 4)

    /* RS_KIND_POSITION */
    DEF_ELEMENT("rs_position1", POSITION, FLOAT_32, false, 1)
    DEF_ELEMENT("rs_position2", POSITION, FLOAT_32, false, 2)
    DEF_ELEMENT("rs_position3", POSITION, FLOAT_32, false, 3)
    DEF_ELEMENT("rs_position4", POSITION, FLOAT_32, false, 4)

    /* RS_KIND_TEXTURE */
    DEF_ELEMENT("rs_texture_coord1", TEXTURE, FLOAT_32, false, 1)
    DEF_ELEMENT("rs_texture_coord2", TEXTURE, FLOAT_32, false, 2)
    DEF_ELEMENT("rs_texture_coord3", TEXTURE, FLOAT_32, false, 3)
    DEF_ELEMENT("rs_texture_coord4", TEXTURE, FLOAT_32, false, 4)

    /* RS_KIND_NORMAL */
    DEF_ELEMENT("rs_normal", NORMAL, FLOAT_32, false, 3)

    /* RS_KIND_POINT_SIZE */
    DEF_ELEMENT("rs_point_size", POINT_SIZE, FLOAT_32, false, 3)

#endif  /* DEF_ELEMENT */

#if 0

/* Element.createPixel(), always normalized */
#ifdef DEF_PIXEL_ELEMENT /* (dk, dt, vsize) */
    DEF_PIXEL_ELEMENT(PIXEL_L, UNSIGNED_8, 1)
    DEF_PIXEL_ELEMENT(PIXEL_A, UNSIGNED_8, 1)
    DEF_PIXEL_ELEMENT(PIXEL_LA, UNSIGNED_8, 2)
    DEF_PIXEL_ELEMENT(PIXEL_RGB, UNSIGNED_8, 3)
    DEF_PIXEL_ELEMENT(PIXEL_RGBA, UNSIGNED_8, 4)
    DEF_PIXEL_ELEMENT(PIXEL_RGB, UNSIGNED_565, 1)
    DEF_PIXEL_ELEMENT(PIXEL_RGBA, UNSIGNED_5551, 1)
    DEF_PIXEL_ELEMENT(PIXEL_RGBA, UNSIGNED_4444, 1)
#   undef DEF_PIXEL_ELEMENT
#endif  /* DEF_PIXEL_ELEMENT */

#endif


#ifdef DEF_PRIMITIVE_DATA_TYPE /* (type, name) */

    DEF_PRIMITIVE_DATA_TYPE(FLOAT_32, "float")
    DEF_PRIMITIVE_DATA_TYPE(SIGNED_8, "char")
    DEF_PRIMITIVE_DATA_TYPE(SIGNED_16, "short")
    DEF_PRIMITIVE_DATA_TYPE(SIGNED_32, "int")
    DEF_PRIMITIVE_DATA_TYPE(UNSIGNED_8, "uchar")
    DEF_PRIMITIVE_DATA_TYPE(UNSIGNED_16, "ushort")
    DEF_PRIMITIVE_DATA_TYPE(UNSIGNED_32, "uint")
    DEF_PRIMITIVE_DATA_TYPE(UNSIGNED_565, "u565")
    DEF_PRIMITIVE_DATA_TYPE(UNSIGNED_5551, "u5551")
    DEF_PRIMITIVE_DATA_TYPE(UNSIGNED_4444, "u4444")

#   undef DEF_PRIMITIVE_DATA_TYPE
#endif  /* DEF_PRIMITIVE_DATA_TYPE */

#ifdef DEF_RS_OBJECT_TYPE /* (type, name) */

    DEF_RS_OBJECT_TYPE(RS_ELEMENT, "rs_element")
    DEF_RS_OBJECT_TYPE(RS_TYPE, "rs_type")
    DEF_RS_OBJECT_TYPE(RS_ALLOCATION, "rs_allocation")
    DEF_RS_OBJECT_TYPE(RS_SAMPLER, "rs_sampler")
    DEF_RS_OBJECT_TYPE(RS_SCRIPT, "rs_script")
    DEF_RS_OBJECT_TYPE(RS_MESH, "rs_mesh")
    DEF_RS_OBJECT_TYPE(RS_PROGRAM_FRAGMENT, "rs_program_fragment")
    DEF_RS_OBJECT_TYPE(RS_PROGRAM_VERTEX, "rs_program_vertex")
    DEF_RS_OBJECT_TYPE(RS_PROGRAM_RASTER, "rs_program_raster")
    DEF_RS_OBJECT_TYPE(RS_PROGRAM_STORE, "rs_program_store")
    DEF_RS_OBJECT_TYPE(RS_FONT, "rs_font")

#   undef DEF_RS_OBJECT_TYPE
#endif  /* DEF_RS_OBJECT_TYPE */

#ifdef DEF_ELEMENT
#   undef DEF_ELEMENT
#endif

#ifdef USE_ELEMENT_DATA_TYPE

#   undef GET_ELEMENT_DATA_TYPE

#   undef ELEMENT_DATA_TYPE_FLOAT_32
#   undef ELEMENT_DATA_TYPE_SIGNED_8
#   undef ELEMENT_DATA_TYPE_SIGNED_16
#   undef ELEMENT_DATA_TYPE_SIGNED_32
#   undef ELEMENT_DATA_TYPE_UNSIGNED_8
#   undef ELEMENT_DATA_TYPE_UNSIGNED_16
#   undef ELEMENT_DATA_TYPE_UNSIGNED_32
#   undef ELEMENT_DATA_TYPE_UNSIGNED_565
#   undef ELEMENT_DATA_TYPE_UNSIGNED_5551
#   undef ELEMENT_DATA_TYPE_UNSIGNED_4444
#   undef ELEMENT_DATA_TYPE_RS_ELEMENT
#   undef ELEMENT_DATA_TYPE_RS_TYPE
#   undef ELEMENT_DATA_TYPE_RS_ALLOCATION
#   undef ELEMENT_DATA_TYPE_RS_SAMPLER
#   undef ELEMENT_DATA_TYPE_RS_SCRIPT
#   undef ELEMENT_DATA_TYPE_RS_MESH
#   undef ELEMENT_DATA_TYPE_RS_PROGRAM_FRAGMENT
#   undef ELEMENT_DATA_TYPE_RS_PROGRAM_VERTEX
#   undef ELEMENT_DATA_TYPE_RS_PROGRAM_RASTER
#   undef ELEMENT_DATA_TYPE_RS_PROGRAM_STORE
#   undef ELEMENT_DATA_TYPE_RS_FONT

#   undef USE_ELEMENT_DATA_TYPE

#endif

#ifdef USE_ELEMENT_DATA_KIND

#   undef GET_ELEMENT_DATA_KIND

#   undef ELEMENT_DATA_KIND_USER
#   undef ELEMENT_DATA_KIND_COLOR
#   undef ELEMENT_DATA_KIND_POSITION
#   undef ELEMENT_DATA_KIND_TEXTURE
#   undef ELEMENT_DATA_KIND_NORMAL
#   undef ELEMENT_DATA_KIND_INDEX
#   undef ELEMENT_DATA_KIND_POINT_SIZE
#   undef ELEMENT_DATA_KIND_PIXEL_L
#   undef ELEMENT_DATA_KIND_PIXEL_A
#   undef ELEMENT_DATA_KIND_PIXEL_LA
#   undef ELEMENT_DATA_KIND_PIXEL_RGB
#   undef ELEMENT_DATA_KIND_PIXEL_RGBA

#   undef USE_ELEMENT_DATA_KIND

#endif
