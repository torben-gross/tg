#include "graphics/tg_spirv.h"

#include "memory/tg_memory.h"
#include "util/tg_string.h"



#define TG_SPIRV_MAGIC_NUMBER    0x07230203
#define TG_SPIRV_MIN_CAPACITY    4



// Source: https://www.khronos.org/registry/spir-v/specs/1.0/SPIRV.html

typedef enum tg_spirv_execution_model
{
    TG_SPIRV_EXECUTION_MODEL_VERTEX                     = 0,
    TG_SPIRV_EXECUTION_MODEL_TESSELLATION_CONTROL       = 1,
    TG_SPIRV_EXECUTION_MODEL_TESSELLATION_EVALUATION    = 2,
    TG_SPIRV_EXECUTION_MODEL_GEOMETRY                   = 3,
    TG_SPIRV_EXECUTION_MODEL_FRAGMENT                   = 4,
    TG_SPIRV_EXECUTION_MODEL_GL_COMPUTE                 = 5,
    TG_SPIRV_EXECUTION_MODEL_KERNEL                     = 6
} tg_spirv_execution_model;

typedef enum tg_spirv_storage_class
{
    TG_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT    = 0,
    TG_SPIRV_STORAGE_CLASS_INPUT               = 1,
    TG_SPIRV_STORAGE_CLASS_UNIFORM             = 2,
    TG_SPIRV_STORAGE_CLASS_OUTPUT              = 3,
    TG_SPIRV_STORAGE_CLASS_WORKGROUP           = 4,
    TG_SPIRV_STORAGE_CLASS_CROSS_WORKGROUP     = 5,
    TG_SPIRV_STORAGE_CLASS_PRIVATE             = 6,
    TG_SPIRV_STORAGE_CLASS_FUNCTION            = 7,
    TG_SPIRV_STORAGE_CLASS_GENERIC             = 8,
    TG_SPIRV_STORAGE_CLASS_PUSH_CONSTANT       = 9,
    TG_SPIRV_STORAGE_CLASS_ATOMIC_COUNTER      = 10,
    TG_SPIRV_STORAGE_CLASS_IMAGE               = 11,
    TG_SPIRV_STORAGE_CLASS_STORAGE_BUFFER      = 12
} tg_spirv_storage_class;

typedef enum tg_spirv_dim
{
    TG_SPIRV_DIM_1D              = 0,
    TG_SPIRV_DIM_2D              = 1,
    TG_SPIRV_DIM_3D              = 2,
    TG_SPIRV_DIM_CUBE            = 3,
    TG_SPIRV_DIM_RECT            = 4,
    TG_SPIRV_DIM_BUFFER          = 5,
    TG_SPIRV_DIM_SUBPASS_DATA    = 6
} tg_spirv_dim;

typedef enum tg_spirv_image_format
{
    TG_SPIRV_IMAGE_FORMAT_UNKNOWN        = 0,
    TG_SPIRV_IMAGE_FORMAT_RGBA32F        = 1,
    TG_SPIRV_IMAGE_FORMAT_RGBA16F        = 2,
    TG_SPIRV_IMAGE_FORMAT_R32F           = 3,
    TG_SPIRV_IMAGE_FORMAT_RGBA8          = 4,
    TG_SPIRV_IMAGE_FORMAT_RGBA8SNORM     = 5,
    TG_SPIRV_IMAGE_FORMAT_RG32F          = 6,
    TG_SPIRV_IMAGE_FORMAT_RG16F          = 7,
    TG_SPIRV_IMAGE_FORMAT_R11G11B10F     = 8,
    TG_SPIRV_IMAGE_FORMAT_R16F           = 9,
    TG_SPIRV_IMAGE_FORMAT_RGBA16         = 10,
    TG_SPIRV_IMAGE_FORMAT_RGB10A2        = 11,
    TG_SPIRV_IMAGE_FORMAT_RG16           = 12,
    TG_SPIRV_IMAGE_FORMAT_RG8            = 13,
    TG_SPIRV_IMAGE_FORMAT_R16            = 14,
    TG_SPIRV_IMAGE_FORMAT_R8             = 15,
    TG_SPIRV_IMAGE_FORMAT_RGBA16SNORM    = 16,
    TG_SPIRV_IMAGE_FORMAT_RG16SNORM      = 17,
    TG_SPIRV_IMAGE_FORMAT_RG8SNORM       = 18,
    TG_SPIRV_IMAGE_FORMAT_R16SNORM       = 19,
    TG_SPIRV_IMAGE_FORMAT_R8SNORM        = 20,
    TG_SPIRV_IMAGE_FORMAT_RGBA32I        = 21,
    TG_SPIRV_IMAGE_FORMAT_RGBA16I        = 22,
    TG_SPIRV_IMAGE_FORMAT_RGBA8I         = 23,
    TG_SPIRV_IMAGE_FORMAT_R32I           = 24,
    TG_SPIRV_IMAGE_FORMAT_RG32I          = 25,
    TG_SPIRV_IMAGE_FORMAT_RG16I          = 26,
    TG_SPIRV_IMAGE_FORMAT_RG8I           = 27,
    TG_SPIRV_IMAGE_FORMAT_R16I           = 28,
    TG_SPIRV_IMAGE_FORMAT_R8I            = 29,
    TG_SPIRV_IMAGE_FORMAT_RGBA32UI       = 30,
    TG_SPIRV_IMAGE_FORMAT_RGBA16UI       = 31,
    TG_SPIRV_IMAGE_FORMAT_RGBA8UI        = 32,
    TG_SPIRV_IMAGE_FORMAT_R32UI          = 33,
    TG_SPIRV_IMAGE_FORMAT_RGB10A2UI      = 34,
    TG_SPIRV_IMAGE_FORMAT_RG32UI         = 35,
    TG_SPIRV_IMAGE_FORMAT_RG16UI         = 36,
    TG_SPIRV_IMAGE_FORMAT_RG8UI          = 37,
    TG_SPIRV_IMAGE_FORMAT_R16UI          = 38,
    TG_SPIRV_IMAGE_FORMAT_R8UI           = 39 
} tg_spirv_image_format;

typedef enum tg_spirv_access_qualifier
{
    TG_SPIRV_ACCESS_QUALIFIER_READ_ONLY     = 0,
    TG_SPIRV_ACCESS_QUALIFIER_WRITE_ONLY    = 1,
    TG_SPIRV_ACCESS_QUALIFIER_READ_WRITE    = 2
} tg_spirv_access_qualifier;

typedef enum tg_spirv_decoration
{
    TG_SPIRV_DECORATION_NONE                              = 0xffffffff,
    TG_SPIRV_DECORATION_RELAXED_PRECISION                 = 0,
    TG_SPIRV_DECORATION_SPEC_ID                           = 1,
    TG_SPIRV_DECORATION_BLOCK                             = 2,
    TG_SPIRV_DECORATION_BUFFER_BLOCK                      = 3,
    TG_SPIRV_DECORATION_ROW_MAJOR                         = 4,
    TG_SPIRV_DECORATION_COL_MAJOR                         = 5,
    TG_SPIRV_DECORATION_ARRAY_STRIDE                      = 6,
    TG_SPIRV_DECORATION_MATRIX_STRIDE                     = 7,
    TG_SPIRV_DECORATION_GLSL_SHARED                       = 8,
    TG_SPIRV_DECORATION_GLSL_PACKED                       = 9,
    TG_SPIRV_DECORATION_C_PACKET                          = 10,
    TG_SPIRV_DECORATION_BUILT_IN                          = 11,
    TG_SPIRV_DECORATION_NO_PERSPECTIVE                    = 13,
    TG_SPIRV_DECORATION_FLAT                              = 14,
    TG_SPIRV_DECORATION_PATCH                             = 15,
    TG_SPIRV_DECORATION_CENTROID                          = 16,
    TG_SPIRV_DECORATION_SAMPLE                            = 17,
    TG_SPIRV_DECORATION_INVARIANT                         = 18,
    TG_SPIRV_DECORATION_RESTRICT                          = 19,
    TG_SPIRV_DECORATION_ALIASED                           = 20,
    TG_SPIRV_DECORATION_VOLATILE                          = 21,
    TG_SPIRV_DECORATION_CONSTANT                          = 22,
    TG_SPIRV_DECORATION_COHERENT                          = 23,
    TG_SPIRV_DECORATION_NON_WRITABLE                      = 24,
    TG_SPIRV_DECORATION_NON_READABLE                      = 25,
    TG_SPIRV_DECORATION_UNIFORM                           = 26,
    TG_SPIRV_DECORATION_SATURATED_CONVERSION              = 28,
    TG_SPIRV_DECORATION_STREAM                            = 29,
    TG_SPIRV_DECORATION_LOCATION                          = 30,
    TG_SPIRV_DECORATION_COMPONENT                         = 31,
    TG_SPIRV_DECORATION_INDEX                             = 32,
    TG_SPIRV_DECORATION_BINDING                           = 33,
    TG_SPIRV_DECORATION_DESCRIPTOR_SET                    = 34,
    TG_SPIRV_DECORATION_OFFSET                            = 35,
    TG_SPIRV_DECORATION_XFB_BUFFER                        = 36,
    TG_SPIRV_DECORATION_XFB_STRIDE                        = 37,
    TG_SPIRV_DECORATION_FUNC_PARAM_ATTR                   = 38,
    TG_SPIRV_DECORATION_FP_ROUNDING_MODE                  = 39,
    TG_SPIRV_DECORATION_FP_FAST_MATH_MODE                 = 40,
    TG_SPIRV_DECORATION_LINKAGE_ATTRIBUTES                = 41,
    TG_SPIRV_DECORATION_NON_CONTRACTION                   = 42,
    TG_SPIRV_DECORATION_INPUT_ATTACHMENT_INDEX            = 43,
    TG_SPIRV_DECORATION_ALIGNMENT                         = 44,
    TG_SPIRV_DECORATION_EXPLICIT_INTERP_AMD               = 4999,
    TG_SPIRV_DECORATION_OVERRIDE_COVERAGE_NV              = 5248,
    TG_SPIRV_DECORATION_PASSTHROUGH_NV                    = 5250,
    TG_SPIRV_DECORATION_VIEWPORT_RELATIVE_NV              = 5252,
    TG_SPIRV_DECORATION_SECONDARY_VIEWPORT_RELATIVE_NV    = 5256
} tg_spirv_decoration;

typedef enum tg_spirv_op
{
    // 3.32.1. Miscellaneous Instructions
    TG_SPIRV_OP_NOP                                              = 0,
    TG_SPIRV_OP_UNDEF                                            = 1,

    // 3.32.2. Debug Instructions
    TG_SPIRV_OP_SOURCE_CONTINUED                                 = 2,
    TG_SPIRV_OP_SOURCE                                           = 3,
    TG_SPIRV_OP_SOURCE_EXTENSION                                 = 4,
    TG_SPIRV_OP_NAME                                             = 5,
    TG_SPIRV_OP_MEMBER_NAME                                      = 6,
    TG_SPIRV_OP_STRING                                           = 7,
    TG_SPIRV_OP_LINE                                             = 8,
    TG_SPIRV_OP_NO_LINE                                          = 317,

    // 3.32.3. Annotation Instructions
    TG_SPIRV_OP_DECORATE                                         = 71,
    TG_SPIRV_OP_MEMBER_DECORATE                                  = 72,
    TG_SPIRV_OP_DECORATION_GROUP                                 = 73,
    TG_SPIRV_OP_GROUP_DECORATE                                   = 74,
    TG_SPIRV_OP_GROUP_MEMBER_DECORATE                            = 75,

    // 3.32.4. Extension Instructions
    TG_SPIRV_OP_EXTENSION                                        = 10,
    TG_SPIRV_OP_EXT_INST_IMPORT                                  = 11,
    TG_SPIRV_OP_EXT_INST                                         = 12,

    // 3.32.5. Mode-Setting Instructions
    TG_SPIRV_OP_MEMORY_MODEL                                     = 14,
    TG_SPIRV_OP_ENTRY_POINT                                      = 15,
    TG_SPIRV_OP_EXECUTION_MODE                                   = 16,
    TG_SPIRV_OP_CAPABILITY                                       = 17,

    // 3.32.6. Type-Declaration Instructions
    TG_SPIRV_OP_TYPE_VOID                                        = 19,
    TG_SPIRV_OP_TYPE_BOOL                                        = 20,
    TG_SPIRV_OP_TYPE_INT                                         = 21,
    TG_SPIRV_OP_TYPE_FLOAT                                       = 22,
    TG_SPIRV_OP_TYPE_VECTOR                                      = 23,
    TG_SPIRV_OP_TYPE_MATRIX                                      = 24,
    TG_SPIRV_OP_TYPE_IMAGE                                       = 25,
    TG_SPIRV_OP_TYPE_SAMPLER                                     = 26,
    TG_SPIRV_OP_TYPE_SAMPLED_IMAGE                               = 27,
    TG_SPIRV_OP_TYPE_ARRAY                                       = 28,
    TG_SPIRV_OP_TYPE_RUNTIME_ARRAY                               = 29,
    TG_SPIRV_OP_TYPE_STRUCT                                      = 30,
    TG_SPIRV_OP_TYPE_OPAQUE                                      = 31,
    TG_SPIRV_OP_TYPE_POINTER                                     = 32,
    TG_SPIRV_OP_TYPE_FUNCTION                                    = 33,
    TG_SPIRV_OP_TYPE_EVENT                                       = 34,
    TG_SPIRV_OP_TYPE_DEVICE_EVENT                                = 35,
    TG_SPIRV_OP_TYPE_RESERVE_ID                                  = 36,
    TG_SPIRV_OP_TYPE_QUEUE                                       = 37,
    TG_SPIRV_OP_TYPE_PIPE                                        = 38,
    TG_SPIRV_OP_TYPE_FORWARD_POINTER                             = 39,

    // 3.32.7.Constant - Creation Instructions
    TG_SPIRV_OP_CONSTANT_TRUE                                    = 41,
    TG_SPIRV_OP_CONSTANT_FALSE                                   = 42,
    TG_SPIRV_OP_CONSTANT                                         = 43,
    TG_SPIRV_OP_CONSTANT_COMPOSITE                               = 44,
    TG_SPIRV_OP_CONSTANT_SAMPLER                                 = 45,
    TG_SPIRV_OP_CONSTANT_NULL                                    = 46,
    TG_SPIRV_OP_SPEC_CONSTANT_TRUE                               = 48,
    TG_SPIRV_OP_SPEC_CONSTANT_FALSE                              = 49,
    TG_SPIRV_OP_SPEC_CONSTANT                                    = 50,
    TG_SPIRV_OP_SPEC_CONSTANT_COMPOSITE                          = 51,
    TG_SPIRV_OP_SPEC_CONSTANT_OP                                 = 52,

    // 3.32.8. Memory Instructions
    TG_SPIRV_OP_VARIABLE                                         = 59,
    TG_SPIRV_OP_IMAGE_TEXEL_POINTER                              = 60,
    TG_SPIRV_OP_LOAD                                             = 61,
    TG_SPIRV_OP_STORE                                            = 62,
    TG_SPIRV_OP_COPY_MEMORY                                      = 63,
    TG_SPIRV_OP_COPY_MEMORY_SIZED                                = 64,
    TG_SPIRV_OP_ACCESS_CHAIN                                     = 65,
    TG_SPIRV_OP_IN_BOUNDS_ACCESS_CHAIN                           = 66,
    TG_SPIRV_OP_PTR_ACCESS_CHAIN                                 = 67,
    TG_SPIRV_OP_ARRAY_LENGTH                                     = 68,
    TG_SPIRV_OP_GENERIC_PTR_MEM_SEMANTICS                        = 69,
    TG_SPIRV_OP_IN_BOUNDS_PTR_ACCESS_CHAIN                       = 70,

    // 3.32.9. Function Instructions
    TG_SPIRV_OP_FUNCTION                                         = 54,
    TG_SPIRV_OP_FUNCTION_PARARMETER                              = 55,
    TG_SPIRV_OP_FUNCTION_END                                     = 56,
    TG_SPIRV_OP_FUNCTION_CALL                                    = 57,

    // 3.32.10. Image Instructions
    TG_SPIRV_OP_SAMPLED_IMAGE                                    = 86,
    TG_SPIRV_OP_IMAGE_SAMPLE_IMPLICIT_LOD                        = 87,
    TG_SPIRV_OP_IMAGE_SAMPLE_EXPLICIT_LOD                        = 88,
    TG_SPIRV_OP_IMAGE_SAMPLE_DREF_IMPLICIT_LOD                   = 89,
    TG_SPIRV_OP_IMAGE_SAMPLE_DREF_EXPLICIT_LOD                   = 90,
    TG_SPIRV_OP_IMAGE_SAMPLE_PROJ_IMPLICIT_LOD                   = 91,
    TG_SPIRV_OP_IMAGE_SAMPLE_PROJ_EXPLICIT_LOD                   = 92,
    TG_SPIRV_OP_IMAGE_SAMPLE_PROJ_DREF_IMPLICIT_LOD              = 93,
    TG_SPIRV_OP_IMAGE_SAMPLE_PROJ_DREF_EXPLICIT_LOD              = 94,
    TG_SPIRV_OP_IMAGE_FETCH                                      = 95,
    TG_SPIRV_OP_IMAGE_GATHER                                     = 96,
    TG_SPIRV_OP_IMAGE_DREF_GATHER                                = 97,
    TG_SPIRV_OP_IMAGE_READ                                       = 98,
    TG_SPIRV_OP_IMAGE_WRITE                                      = 99,
    TG_SPIRV_OP_IMAGE                                            = 100,
    TG_SPIRV_OP_IMAGE_QUERY_FORMAT                               = 101,
    TG_SPIRV_OP_IMAGE_QUERY_ORDER                                = 102,
    TG_SPIRV_OP_IMAGE_QUERY_SIZE_LOD                             = 103,
    TG_SPIRV_OP_IMAGE_QUERY_SIZE                                 = 104,
    TG_SPIRV_OP_IMAGE_QUERY_LOD                                  = 105,
    TG_SPIRV_OP_IMAGE_QUERY_LEVELS                               = 106,
    TG_SPIRV_OP_IMAGE_QUERY_SAMPELS                              = 107,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_IMPLICIT_LOD                 = 305,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_EXPLICIT_LOD                 = 306,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_DREF_IMPLICIT_LOD            = 307,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_DREF_EXPLICIT_LOD            = 308,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_PROJ_IMPLICIT_LOD            = 309,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_PROJ_EXPLICIT_LOD            = 310,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_PROJ_DREF_IMPLICIT_LOD       = 311,
    TG_SPIRV_OP_IMAGE_SPARSE_SAMPLE_PROJ_DREF_EXPLICIT_LOD       = 312,
    TG_SPIRV_OP_IMAGE_SPARSE_FETCH                               = 313,
    TG_SPIRV_OP_IMAGE_SPARSE_GATHER                              = 314,
    TG_SPIRV_OP_IMAGE_SPARSE_DREF_GATHER                         = 315,
    TG_SPIRV_OP_IMAGE_SPARSE_TEXELS_RESIDENT                     = 316,
    TG_SPIRV_OP_IMAGE_SPARSE_READ                                = 320,

    // 3.32.11. Conversion Instructions
    TG_SPIRV_OP_CONVERT_F_TO_U                                   = 109,
    TG_SPIRV_OP_CONVERT_F_TO_S                                   = 110,
    TG_SPIRV_OP_CONVERT_S_TO_F                                   = 111,
    TG_SPIRV_OP_CONVERT_U_TO_F                                   = 112,
    TG_SPIRV_OP_U_CONVERT                                        = 113,
    TG_SPIRV_OP_S_CONVERT                                        = 114,
    TG_SPIRV_OP_F_CONVERT                                        = 115,
    TG_SPIRV_OP_QUANTIZE_TO_F16                                  = 116,
    TG_SPIRV_OP_CONVERT_PTR_TO_U                                 = 117,
    TG_SPIRV_OP_SAT_CONVERT_S_TO_U                               = 118,
    TG_SPIRV_OP_SAT_CONVERT_U_TO_S                               = 119,
    TG_SPIRV_OP_GENERIC_CAST_TO_PTR                              = 122,
    TG_SPIRV_OP_CONVERT_U_TO_PTR                                 = 120,
    TG_SPIRV_OP_PTR_CAST_TO_GENERIC                              = 121,
    TG_SPIRV_OP_GENERIC_CAST_TO_PTR_EXPLICIT                     = 123,
    TG_SPIRV_OP_BITCAST                                          = 124,

    // 3.32.12. Composite Instructions
    TG_SPIRV_OP_VECTOR_EXTRACT_DYNAMIC                           = 77,
    TG_SPIRV_OP_VECTOR_INSERT_DYNAMIC                            = 78,
    TG_SPIRV_OP_VECTOR_SHUFFLE                                   = 79,
    TG_SPIRV_OP_COMPOSITE_CONSTRUCT                              = 80,
    TG_SPIRV_OP_COMPOSITE_EXTRACT                                = 81,
    TG_SPIRV_OP_COMPOSITE_INSERT                                 = 82,
    TG_SPIRV_OP_COPY_OBJECT                                      = 83,
    TG_SPIRV_OP_TRANSPOSE                                        = 84,

    // 3.32.13. Arithmetic Instructions
    TG_SPIRV_OP_S_NEGATE                                         = 126,
    TG_SPIRV_OP_F_NEGATE                                         = 127,
    TG_SPIRV_OP_I_ADD                                            = 128,
    TG_SPIRV_OP_F_ADD                                            = 129,
    TG_SPIRV_OP_I_SUB                                            = 130,
    TG_SPIRV_OP_F_SUB                                            = 131,
    TG_SPIRV_OP_I_MUL                                            = 132,
    TG_SPIRV_OP_F_MUL                                            = 133,
    TG_SPIRV_OP_U_DIV                                            = 134,
    TG_SPIRV_OP_S_DIV                                            = 135,
    TG_SPIRV_OP_F_DIV                                            = 136,
    TG_SPIRV_OP_U_MOD                                            = 137,
    TG_SPIRV_OP_S_REM                                            = 138,
    TG_SPIRV_OP_S_MOD                                            = 139,
    TG_SPIRV_OP_F_REM                                            = 140,
    TG_SPIRV_OP_F_MOD                                            = 141,
    TG_SPIRV_OP_VECTOR_TIMES_SCALER                              = 142,
    TG_SPIRV_OP_MATRIX_TIMES_SCALER                              = 143,
    TG_SPIRV_OP_VECTOR_TIMES_MATRIX                              = 144,
    TG_SPIRV_OP_MATRIX_TIMES_VECTOR                              = 145,
    TG_SPIRV_OP_MATRIX_TIMES_MATRIX                              = 146,
    TG_SPIRV_OP_OUTER_PRODUCT                                    = 147,
    TG_SPIRV_OP_DOT                                              = 148,
    TG_SPIRV_OP_I_ADD_CARRY                                      = 149,
    TG_SPIRV_OP_I_SUB_BORROW                                     = 150,
    TG_SPIRV_OP_U_MUL_EXTENDED                                   = 151,
    TG_SPIRV_OP_S_MUL_EXTENDED                                   = 152,

    // 3.32.14. Bit Instructions
    TG_SPIRV_OP_SHIFT_RIGHT_LOGICAL                              = 194,
    TG_SPIRV_OP_SHIFT_RIGHT_ARITHMETIC                           = 195,
    TG_SPIRV_OP_SHIFT_LEFT_LOGICAL                               = 196,
    TG_SPIRV_OP_BITWISE_OR                                       = 197,
    TG_SPIRV_OP_BITWISE_COR                                      = 198,
    TG_SPIRV_OP_BITWISE_AND                                      = 199,
    TG_SPIRV_OP_NOT                                              = 200,
    TG_SPIRV_OP_BIT_FIELD_INSERT                                 = 201,
    TG_SPIRV_OP_BIT_FIELD_S_EXTRACT                              = 202,
    TG_SPIRV_OP_BIT_FIELD_U_EXTRACT                              = 203,
    TG_SPIRV_OP_BIT_REVERSE                                      = 204,
    TG_SPIRV_OP_BIT_COUNT                                        = 205,

    // 3.32.15. Relational and Logical Instructions
    TG_SPIRV_OP_ANY                                              = 154,
    TG_SPIRV_OP_ALL                                              = 155,
    TG_SPIRV_OP_IS_NAN                                           = 156,
    TG_SPIRV_OP_IS_INF                                           = 157,
    TG_SPIRV_OP_IS_FINITE                                        = 158,
    TG_SPIRV_OP_IS_NORMAL                                        = 159,
    TG_SPIRV_OP_SIGN_BIT_SET                                     = 160,
    TG_SPIRV_OP_LESS_OR_GREATER                                  = 161,
    TG_SPIRV_OP_ORDERED                                          = 162,
    TG_SPIRV_OP_UNORDERED                                        = 163,
    TG_SPIRV_OP_LOGICAL_EQUAL                                    = 164,
    TG_SPIRV_OP_LOGICAL_NOT_EQUAL                                = 165,
    TG_SPIRV_OP_LOGICAL_OR                                       = 166,
    TG_SPIRV_OP_LOGICAL_AND                                      = 167,
    TG_SPIRV_OP_LOGICAL_NOT                                      = 168,
    TG_SPIRV_OP_SELECT                                           = 169,
    TG_SPIRV_OP_EQUAL                                            = 170,
    TG_SPIRV_OP_I_NOT_EQUAL                                      = 171,
    TG_SPIRV_OP_U_GREATER_THAN                                   = 172,
    TG_SPIRV_OP_S_GREATER_THAN                                   = 173,
    TG_SPIRV_OP_U_GREATER_THAN_EQUAL                             = 174,
    TG_SPIRV_OP_S_GREATER_THAN_EQUAL                             = 175,
    TG_SPIRV_OP_U_LESS_THAN                                      = 176,
    TG_SPIRV_OP_S_LESS_THAN                                      = 177,
    TG_SPIRV_OP_U_LESS_THAN_EQUAL                                = 178,
    TG_SPIRV_OP_S_LESS_THAN_EQUAL                                = 179,
    TG_SPIRV_OP_F_ORD_EQUAL                                      = 180,
    TG_SPIRV_OP_F_UNORD_EQUAL                                    = 181,
    TG_SPIRV_OP_F_ORD_NOT_EQUAL                                  = 182,
    TG_SPIRV_OP_F_UNORD_NOT_EQUAL                                = 183,
    TG_SPIRV_OP_F_ORD_LESS_THAN                                  = 184,
    TG_SPIRV_OP_F_UNORD_LESS_THAN                                = 185,
    TG_SPIRV_OP_F_ORD_GREATER_THAN                               = 186,
    TG_SPIRV_OP_F_UNORD_GREATER_THAN                             = 187,
    TG_SPIRV_OP_F_ORD_LESS_THAN_EQUAL                            = 188,
    TG_SPIRV_OP_F_UNORD_LESS_THAN_EQUAL                          = 189,
    TG_SPIRV_OP_F_ORD_GREATER_THAN_EQUAL                         = 190,
    TG_SPIRV_OP_F_UNORD_GREATER_THAN_EQUAL                       = 191,

    // 3.32.16. Derivative Instructions
    TG_SPIRV_OP_D_DPX                                            = 207,
    TG_SPIRV_OP_D_DPY                                            = 208,
    TG_SPIRV_OP_F_WIDTH                                          = 209,
    TG_SPIRV_OP_D_PDX_FINE                                       = 210,
    TG_SPIRV_OP_D_PDY_FINE                                       = 211,
    TG_SPIRV_OP_F_WIDTH_FINE                                     = 212,
    TG_SPIRV_OP_D_PDX_COARSE                                     = 213,
    TG_SPIRV_OP_D_PDY_COARSE                                     = 214,
    TG_SPIRV_OP_F_WIDTH_COARSE                                   = 215,

    // 3.32.17. Control-Flow Instructions
    TG_SPIRV_OP_PHI                                              = 245,
    TG_SPIRV_OP_LOOP_MERGE                                       = 246,
    TG_SPIRV_OP_SELECTION_MERGE                                  = 247,
    TG_SPIRV_OP_LABEL                                            = 248,
    TG_SPIRV_OP_BRANCH                                           = 249,
    TG_SPIRV_OP_BRANCH_CONDITIONAL                               = 250,
    TG_SPIRV_OP_SWITCH                                           = 251,
    TG_SPIRV_OP_KILL                                             = 252,
    TG_SPIRV_OP_RETURN                                           = 253,
    TG_SPIRV_OP_RETURN_VALUE                                     = 254,
    TG_SPIRV_OP_UNREACHABLE                                      = 255,
    TG_SPIRV_OP_LIFETIME_START                                   = 256,
    TG_SPIRV_OP_LIFETIME_STOP                                    = 257,

    // 3.32.18. Atomic Instructions,
    TG_SPIRV_OP_ATOMIC_LOAD                                      = 227,
    TG_SPIRV_OP_ATOMIC_STORE                                     = 228,
    TG_SPIRV_OP_ATOMIC_EXCHANGE                                  = 229,
    TG_SPIRV_OP_ATOMIC_COMPARE_EXCHANGE                          = 230,
    TG_SPIRV_OP_ATOMIC_COMPARE_EXCHANGE_WEAK                     = 231,
    TG_SPIRV_OP_ATOMIC_INCREMENT                                 = 232,
    TG_SPIRV_OP_ATOMIC_DECREMENT                                 = 233,
    TG_SPIRV_OP_ATOMIC_ADD                                       = 234,
    TG_SPIRV_OP_ATOMIC_SUB                                       = 235,
    TG_SPIRV_OP_ATOMIC_S_MIN                                     = 236,
    TG_SPIRV_OP_ATOMIC_U_MIN                                     = 237,
    TG_SPIRV_OP_ATOMIC_S_MAX                                     = 238,
    TG_SPIRV_OP_ATOMIC_U_MAX                                     = 239,
    TG_SPIRV_OP_ATOMIC_AND                                       = 240,
    TG_SPIRV_OP_ATOMIC_OR                                        = 241,
    TG_SPIRV_OP_ATOMIC_XOR                                       = 242,
    TG_SPIRV_OP_ATOMIC_FLAG_TEST_AND_SET                         = 318,
    TG_SPIRV_OP_ATOMIC_FLAG_CLEAR                                = 319,

    // 3.32.19. Primitive Instructions,
    TG_SPIRV_OP_EMIT_VERTEX                                      = 218,
    TG_SPIRV_OP_END_PRIMITIVE                                    = 219,
    TG_SPIRV_OP_EMIT_STREAM_VERTEX                               = 220,
    TG_SPIRV_OP_END_STREAM_PRIMITIVE                             = 221,

    // 3.32.20. Barrier Instructions
    TG_SPIRV_OP_CONTROL_BARRIER                                  = 224,
    TG_SPIRV_OP_MEMORY_BARRIER                                   = 225,

    // 3.32.21. Group Instructions
    TG_SPIRV_OP_GROUP_ASYNC_COPY                                 = 259,
    TG_SPIRV_OP_GROUP_WAIT_EVENTS                                = 260,
    TG_SPIRV_OP_GROUP_ALL                                        = 261,
    TG_SPIRV_OP_GROUP_ANY                                        = 262,
    TG_SPIRV_OP_GROUP_BROADCAST                                  = 263,
    TG_SPIRV_OP_GROUP_I_ADD                                      = 264,
    TG_SPIRV_OP_GROUP_F_ADD                                      = 265,
    TG_SPIRV_OP_GROUP_F_MIN                                      = 266,
    TG_SPIRV_OP_GROUP_U_MIN                                      = 267,
    TG_SPIRV_OP_GROUP_S_MIN                                      = 268,
    TG_SPIRV_OP_GROUP_F_MAX                                      = 269,
    TG_SPIRV_OP_GROUP_U_MAX                                      = 270,
    TG_SPIRV_OP_GROUP_S_MAX                                      = 271,
    TG_SPIRV_OP_SUBGROUP_BALLOUT_KHR                             = 4421,
    TG_SPIRV_OP_SUBGROUP_FIRST_INVOCATION_KHR                    = 4422,
    TG_SPIRV_OP_SUBGROUP_READ_INVOCATION_KHR                     = 4432,
    TG_SPIRV_OP_GROUP_I_ADD_NON_UNIFORM_AMD                      = 5000,
    TG_SPIRV_OP_GROUP_F_ADD_NON_UNIFORM_AMD                      = 5001,
    TG_SPIRV_OP_F_MIN_NON_UNIFORM_AMD                            = 5002,
    TG_SPIRV_OP_U_MIN_NON_UNIFORM_AMD                            = 5003,
    TG_SPIRV_OP_S_MIN_NON_UNIFORM_AMD                            = 5004,
    TG_SPIRV_OP_F_MAX_NON_UNIFORM_AMD                            = 5002,
    TG_SPIRV_OP_U_MAX_NON_UNIFORM_AMD                            = 5003,
    TG_SPIRV_OP_S_MAX_NON_UNIFORM_AMD                            = 5004,

    // 3.32.22. Device-Side Enqueue Instructions
    TG_SPIRV_OP_ENQUEUE_MARKER                                   = 291,
    TG_SPIRV_OP_ENQUEUE_KERNEL                                   = 292,
    TG_SPIRV_OP_GET_KERNEL_N_DRANGE_SUB_GROUP_COUNT              = 293,
    TG_SPIRV_OP_GET_KERNEL_N_DRANGE_MAX_SUB_GROUP_SIZE           = 294,
    TG_SPIRV_OP_GET_KERNEL_WORK_GROUP_SIZE                       = 295,
    TG_SPIRV_OP_GET_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE    = 296,
    TG_SPIRV_OP_RETAIN_EVENT                                     = 297,
    TG_SPIRV_OP_RELEASE_EVENT                                    = 298,
    TG_SPIRV_OP_CREATE_USER_EVENT                                = 299,
    TG_SPIRV_OP_IS_VALID_EVENT                                   = 300,
    TG_SPIRV_OP_SET_USER_EVENT_STATUS                            = 301,
    TG_SPIRV_OP_CAPTURE_EVENT_PROFILING_INFO                     = 302,
    TG_SPIRV_OP_GET_DEFAULT_QUEUE                                = 303,
    TG_SPIRV_OP_BUILD_N_D_RANGE                                  = 304,

    // 3.32.23. Pipe Instructions
    TG_SPIRV_OP_READ_PIPE                                        = 274,
    TG_SPIRV_OP_WRITE_PIPE                                       = 275,
    TG_SPIRV_OP_RESERVED_READ_PIPE                               = 276,
    TG_SPIRV_OP_RESERVED_WRITE_PIPE                              = 277,
    TG_SPIRV_OP_RESERVED_READ_PIPE_PACKETS                       = 278,
    TG_SPIRV_OP_RESERVED_WRITE_PIPE_PACKETS                      = 279,
    TG_SPIRV_OP_COMMIT_READ_PIPE                                 = 280,
    TG_SPIRV_OP_COMMIT_WRITE_PIPE                                = 281,
    TG_SPIRV_OP_IS_VALID_RESERVE_ID                              = 282,
    TG_SPIRV_OP_GET_NUM_PIPE_PACKETS                             = 283,
    TG_SPIRV_OP_GET_MAX_PIPE_PACKETS                             = 284,
    TG_SPIRV_OP_GROUP_RESERVE_READ_PIPE_PACKETS                  = 285,
    TG_SPIRV_OP_GROUP_RESERVE_WRITE_PIPE_PACKETS                 = 286,
    TG_SPIRV_OP_GROUP_COMMIT_READ_PACKETS                        = 287,
    TG_SPIRV_OP_GROUP_COMMIT_WRITE_PACKETS                       = 288,
} tg_spirv_op;



void tg_spirv_internal_find_type(u32 word_count, const u32* p_words, u32 id, tg_spirv_op* p_type, u32* p_literal)
{
    u32 processed_word_count = 0;
    while (processed_word_count < word_count)
    {
        const u32 opcode = p_words[processed_word_count];
        const tg_spirv_op op = opcode & 0xffff;
        const u16 op_word_count = opcode >> 16;

        if (op >= TG_SPIRV_OP_TYPE_VOID && op <= TG_SPIRV_OP_TYPE_FORWARD_POINTER && p_words[processed_word_count + 1] == id)
        {
            *p_type = op;
            switch (op)
            {
            case TG_SPIRV_OP_TYPE_INT:
            {
                const u32 width = p_words[processed_word_count + 2];
                const u32 signedness = p_words[processed_word_count + 3];
                *p_literal = width | (signedness << 16);
            } break;
            case TG_SPIRV_OP_TYPE_FLOAT:
            {
                const u32 width = p_words[processed_word_count + 2];
                *p_literal = width;
            } break;
            default: *p_literal = 0; break;
            }
            return;
        }

        processed_word_count += op_word_count;
    }
    TG_INVALID_CODEPATH();
}

void tg_spirv_internal_fill_global_resource(u32 word_count, const u32* p_words, u32 id, tg_spirv_global_resource* p_resource)
{
    u32 processed_word_count = 0;
    while (processed_word_count < word_count)
    {
        const u32 opcode = p_words[processed_word_count];
        const tg_spirv_op op = opcode & 0xffff;
        const u16 op_word_count = opcode >> 16;

        switch (op)
        {
        case TG_SPIRV_OP_DECORATE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_decoration decoration = p_words[processed_word_count + 2];
                switch (decoration)
                {
                case TG_SPIRV_DECORATION_BLOCK: p_resource->type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_UNIFORM_BUFFER; break;
                case TG_SPIRV_DECORATION_BUFFER_BLOCK: p_resource->type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_BUFFER; break;
                case TG_SPIRV_DECORATION_BINDING: p_resource->binding = p_words[processed_word_count + 3]; break;
                case TG_SPIRV_DECORATION_DESCRIPTOR_SET: p_resource->descriptor_set = p_words[processed_word_count + 3]; break;
                }
            }
        } break;
        case TG_SPIRV_OP_TYPE_SAMPLED_IMAGE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id && p_resource->type == TG_SPIRV_GLOBAL_RESOURCE_TYPE_INVALID)
            {
                p_resource->type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
            }
        } break;
        case TG_SPIRV_OP_TYPE_POINTER:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_storage_class storage_class = p_words[processed_word_count + 2];
                if (storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM || storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT)
                {
                    const u32 pointed_to_id = p_words[processed_word_count + 3];
                    tg_spirv_internal_fill_global_resource(word_count, p_words, pointed_to_id, p_resource);
                }
                else
                {
                    TG_INVALID_CODEPATH();
                }
            }
        } break;
        }

        processed_word_count += op_word_count;
    }
}

void tg_spirv_internal_fill_input_resource(u32 word_count, const u32* p_words, u32 id, tg_spirv_input_resource* p_resource)
{
    u32 processed_word_count = 0;
    while (processed_word_count < word_count)
    {
        const u32 opcode = p_words[processed_word_count];
        const tg_spirv_op op = opcode & 0xffff;
        const u16 op_word_count = opcode >> 16;

        switch (op)
        {
        case TG_SPIRV_OP_DECORATE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_decoration decoration = p_words[processed_word_count + 2];
                switch (decoration)
                {
                case TG_SPIRV_DECORATION_LOCATION:
                {
                    p_resource->location = p_words[processed_word_count + 3];
                } break;
                case TG_SPIRV_DECORATION_OFFSET:
                {
                    p_resource->offset = p_words[processed_word_count + 3];
                } break;
                }
            }
        } break;
        case TG_SPIRV_OP_TYPE_INT:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id && p_resource->format == TG_SPIRV_INPUT_FORMAT_INVALID)
            {
                TG_ASSERT(p_words[processed_word_count + 2] == 32);
                const b32 is_signed = p_words[processed_word_count + 3];
                if (is_signed)
                {
                    p_resource->format = TG_SPIRV_INPUT_FORMAT_R32_SINT;
                }
                else
                {
                    p_resource->format = TG_SPIRV_INPUT_FORMAT_R32_UINT;
                }
            }
        } break;
        case TG_SPIRV_OP_TYPE_FLOAT:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id && p_resource->format == TG_SPIRV_INPUT_FORMAT_INVALID)
            {
                TG_ASSERT(p_words[processed_word_count + 2] == 32);
                p_resource->format = TG_SPIRV_INPUT_FORMAT_R32_SFLOAT;
            }
        } break;
        case TG_SPIRV_OP_TYPE_VECTOR:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id && p_resource->format == TG_SPIRV_INPUT_FORMAT_INVALID)
            {
                u32 literal;
                tg_spirv_op component_type;
                tg_spirv_internal_find_type(word_count, p_words, p_words[processed_word_count + 2], &component_type, &literal);
                const u32 component_number = p_words[processed_word_count + 3];
                switch (component_type)
                {
                case TG_SPIRV_OP_TYPE_INT:
                {
                    TG_ASSERT((literal & 0xffff) == 32);
                    const b32 is_signed = literal >> 16;
                    switch (component_number)
                    {
                    case 2: p_resource->format = is_signed ? TG_SPIRV_INPUT_FORMAT_R32G32_SINT : TG_SPIRV_INPUT_FORMAT_R32G32_UINT; break;
                    case 3: p_resource->format = is_signed ? TG_SPIRV_INPUT_FORMAT_R32G32B32_SINT : TG_SPIRV_INPUT_FORMAT_R32G32B32_UINT; break;
                    case 4: p_resource->format = is_signed ? TG_SPIRV_INPUT_FORMAT_R32G32B32A32_SINT : TG_SPIRV_INPUT_FORMAT_R32G32B32A32_UINT; break;
                    default: TG_INVALID_CODEPATH(); break;
                    }
                } break;
                case TG_SPIRV_OP_TYPE_FLOAT:
                {
                    TG_ASSERT(literal == 32);
                    switch (component_number)
                    {
                    case 2: p_resource->format = TG_SPIRV_INPUT_FORMAT_R32G32_SFLOAT; break;
                    case 3: p_resource->format = TG_SPIRV_INPUT_FORMAT_R32G32B32_SFLOAT; break;
                    case 4: p_resource->format = TG_SPIRV_INPUT_FORMAT_R32G32B32A32_SFLOAT; break;
                    default: TG_INVALID_CODEPATH(); break;
                    }
                } break;
                default: TG_INVALID_CODEPATH(); break;
                }
            }
        } break;
        case TG_SPIRV_OP_TYPE_POINTER:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_storage_class storage_class = p_words[processed_word_count + 2];
                if (storage_class == TG_SPIRV_STORAGE_CLASS_INPUT)
                {
                    const u32 pointed_to_id = p_words[processed_word_count + 3];
                    tg_spirv_internal_fill_input_resource(word_count, p_words, pointed_to_id, p_resource);
                }
                else
                {
                    TG_INVALID_CODEPATH();
                }
            }
        } break;
        }

        processed_word_count += op_word_count;
    }
}



void tg_spirv_fill_layout(u32 word_count, const u32* p_words, tg_spirv_layout* p_layout)
{
    TG_ASSERT(word_count && p_words && p_layout);

    u32 processed_word_count = 0;
    TG_ASSERT(p_words[0] == TG_SPIRV_MAGIC_NUMBER);
    processed_word_count += 5;

    const u32 mask =
        (1 << TG_SPIRV_OP_CAPABILITY)       |
        (1 << TG_SPIRV_OP_EXTENSION)        |
        (1 << TG_SPIRV_OP_EXT_INST_IMPORT)  |
        (1 << TG_SPIRV_OP_MEMORY_MODEL)     |
        (1 << TG_SPIRV_OP_ENTRY_POINT)      |
        (1 << TG_SPIRV_OP_EXECUTION_MODE)   |
        (1 << TG_SPIRV_OP_STRING)           |
        (1 << TG_SPIRV_OP_SOURCE_EXTENSION) |
        (1 << TG_SPIRV_OP_SOURCE)           |
        (1 << TG_SPIRV_OP_SOURCE_CONTINUED);
    while (processed_word_count < word_count)
    {
        const tg_spirv_op op = p_words[processed_word_count] & 0xffff;

        if (((1 << op) & mask) == 0)
        {
            p_words = &p_words[processed_word_count];
            word_count -= processed_word_count;
            processed_word_count = 0;
            break;
        }
        else
        {
            const u16 op_word_count = p_words[processed_word_count] >> 16;
            if (op == TG_SPIRV_OP_ENTRY_POINT)
            {
                switch (p_words[processed_word_count + 1])
                {
                case TG_SPIRV_EXECUTION_MODEL_VERTEX:     p_layout->shader_type = TG_SPIRV_SHADER_TYPE_VERTEX;   break;
                case TG_SPIRV_EXECUTION_MODEL_FRAGMENT:   p_layout->shader_type = TG_SPIRV_SHADER_TYPE_FRAGMENT; break;
                case TG_SPIRV_EXECUTION_MODEL_GL_COMPUTE: p_layout->shader_type = TG_SPIRV_SHADER_TYPE_COMPUTE;  break;
                default: TG_INVALID_CODEPATH(); break;
                }
                const char* p_entry_point_name = (char*)&p_words[processed_word_count + 3];
                const u32 entry_point_length = tg_string_length(p_entry_point_name);
                TG_ASSERT(entry_point_length + 1 <= TG_SPIRV_MAX_NAME);
                tg_memory_copy((u64)entry_point_length + 1, p_entry_point_name, p_layout->p_entry_point_name);
            }
            processed_word_count += op_word_count;
        }
    }

    while (processed_word_count < word_count)
    {
        const tg_spirv_op opcode_enumerant = p_words[processed_word_count] & 0xffff;
        const u16 op_word_count = p_words[processed_word_count] >> 16;

        if (opcode_enumerant == TG_SPIRV_OP_VARIABLE)
        {
            const u32 points_to_id = p_words[processed_word_count + 1];
            const u32 variable_id = p_words[processed_word_count + 2];
            const tg_spirv_storage_class storage_class = p_words[processed_word_count + 3];

            if (storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT || storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM)
            {
                TG_ASSERT(p_layout->global_resource_count < TG_SPIRV_MAX_GLOBALS);
                tg_spirv_internal_fill_global_resource(word_count, p_words, variable_id, &p_layout->p_global_resources[p_layout->global_resource_count]);
                tg_spirv_internal_fill_global_resource(word_count, p_words, points_to_id, &p_layout->p_global_resources[p_layout->global_resource_count]);
                p_layout->global_resource_count++;
            }
            else if (storage_class == TG_SPIRV_STORAGE_CLASS_INPUT)
            {
                TG_ASSERT(p_layout->input_resource_count < TG_SPIRV_MAX_INPUTS);
                tg_spirv_internal_fill_input_resource(word_count, p_words, variable_id, &p_layout->p_input_resources[p_layout->input_resource_count]);
                tg_spirv_internal_fill_input_resource(word_count, p_words, points_to_id, &p_layout->p_input_resources[p_layout->input_resource_count]);
                p_layout->input_resource_count++;
            }
        }

        processed_word_count += op_word_count;
    }
}
