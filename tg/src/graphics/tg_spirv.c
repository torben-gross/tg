#include "graphics/vulkan/tgvk_core.h"
#include "memory/tg_memory.h"
#include "util/tg_qsort.h"
#include "util/tg_string.h"



#define TG_SPIRV_MAGIC_NUMBER    0x07230203



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
    TG_SPIRV_OP_EXECUTION_MODEL                                   = 16,
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



typedef struct tg_spirv_op_decorate
{
    u32    id_target;
    tg_spirv_decoration decoration;
    u32    value;
} tg_spirv_op_decorate;

typedef struct tg_spirv_op_referenceable
{
    tg_spirv_op    op;
    union
    {
        struct
        {
            u32    width;
            u32    signedness;
        } type_int;
        struct
        {
            u32    width;
        } type_float;
        struct
        {
            u32    id_component_type;
            u32    component_count;
        } type_vector;
        struct
        {
            u32    id_column_type;
            u32    column_count;
        } type_matrix;
        struct
        {
            u32    id_element_type;
            u32    id_length;
        } type_array;
        struct
        {
            tg_spirv_storage_class storage_class;
            u32    id_return_type;
        } type_pointer;
        struct
        {
            u32    id_result_type;
            u32    value;
        } constant;
    };
} tg_spirv_op_referenceable;

typedef struct tg_spirv_op_variable
{
    u32    id_result_type;
    u32    id;
    tg_spirv_storage_class storage_class;
} tg_spirv_op_variable;



static i32 tg__compare_by_location(const tg_spirv_inout_resource* p_v0, const tg_spirv_inout_resource* p_v1, void* p_user_data)
{
    const i32 result = p_v0->location - p_v1->location;
    TG_UNUSED(p_user_data);
    return result;
}

static tg_vertex_input_attribute_format tg__resolve_inout_resource_format(u32 op_decorate_count, const tg_spirv_op_decorate* p_op_decorate_buffer, u32 op_referencable_count, const tg_spirv_op_referenceable* p_op_referenceable_buffer, const tg_spirv_op_referenceable* p)
{
#ifdef TG_DEBUG

#define TG_SPIRV_OP_REFERENCEABLE_INIT(result_id_word_offset)                                   \
    (p_words[processed_word_count + (result_id_word_offset)] <= op_referencable_count)          \
        ? (&p_op_referenceable_buffer[p_words[processed_word_count + (result_id_word_offset)]]) \
        : (*(tg_spirv_op_referenceable**)0);                                                    \
    p->op = op

#define TG_SPIRV_OP_REFERENCEABLE_GET(id)    \
    (((id) < op_referencable_count)          \
        ? (&p_op_referenceable_buffer[(id)]) \
        : (*(tg_spirv_op_referenceable**)0))

#else

#define TG_SPIRV_OP_REFERENCEABLE_INIT(result_id_word_offset)                            \
    &p_op_referenceable_buffer[p_words[processed_word_count + (result_id_word_offset)]]; \
    p->op = op

#define TG_SPIRV_OP_REFERENCEABLE_GET(id) \
    (&p_op_referenceable_buffer[(id)])

#endif

    tg_vertex_input_attribute_format result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_INVALID;

    switch (p->op)
    {
    case TG_SPIRV_OP_TYPE_POINTER:
    {
        TG_ASSERT(TG_SPIRV_OP_REFERENCEABLE_GET(p->type_pointer.id_return_type)->op != TG_SPIRV_OP_NOP);
        TG_ASSERT(TG_SPIRV_OP_REFERENCEABLE_GET(p->type_pointer.id_return_type)->op != TG_SPIRV_OP_TYPE_POINTER);
        result = tg__resolve_inout_resource_format(op_decorate_count, p_op_decorate_buffer, op_referencable_count, p_op_referenceable_buffer, TG_SPIRV_OP_REFERENCEABLE_GET(p->type_pointer.id_return_type));
    } break;
    case TG_SPIRV_OP_TYPE_INT:
    {
        TG_ASSERT(p->type_int.width == 32);
        result = p->type_int.signedness == 1 ? TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT : TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT;
    } break;
    case TG_SPIRV_OP_TYPE_FLOAT:
    {
        TG_ASSERT(p->type_float.width == 32);
        result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT;
    } break;
    case TG_SPIRV_OP_TYPE_VECTOR:
    {
        const tg_vertex_input_attribute_format element_format = tg__resolve_inout_resource_format(op_decorate_count, p_op_decorate_buffer, op_referencable_count, p_op_referenceable_buffer, TG_SPIRV_OP_REFERENCEABLE_GET(p->type_vector.id_component_type));
        const u32 component_count = p->type_vector.component_count;

        switch (element_format)
        {
        case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT:
        {
            switch (component_count)
            {
            case 2: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT;       break;
            case 3: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;    break;
            case 4: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT; break;

            default: TG_INVALID_CODEPATH(); break;
            }
        } break;
        case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT:
        {
            switch (component_count)
            {
            case 2: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SINT;       break;
            case 3: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SINT;    break;
            case 4: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SINT; break;

            default: TG_INVALID_CODEPATH(); break;
            }
        } break;
        case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT:
        {
            switch (component_count)
            {
            case 2: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_UINT;       break;
            case 3: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_UINT;    break;
            case 4: result = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_UINT; break;

            default: TG_INVALID_CODEPATH(); break;
            }
        } break;

        default: TG_INVALID_CODEPATH(); break;
        }
    } break;
        // TODO: Each array element (E.G. element in array OR column in matrix) is a separate
        // location. The first element of the array has the specified location (with the location
        // value), and each subsequent element increases that location by 1.
    case TG_SPIRV_OP_TYPE_MATRIX:
    case TG_SPIRV_OP_TYPE_ARRAY:
    default: TG_INVALID_CODEPATH(); break;
    }

    return result;
}



void tg_spirv_fill_layout(u32 word_count, const u32* p_words, TG_OUT tg_spirv_layout* p_layout)
{
    TG_ASSERT(word_count && p_words && p_layout);
    TG_ASSERT(p_words[0] == TG_SPIRV_MAGIC_NUMBER);

    // ENTRY POINT

    u32 processed_word_count = 5;
    while (processed_word_count < word_count)
    {
        const tg_spirv_op op = p_words[processed_word_count] & 0xffff;
        const u16 op_word_count = p_words[processed_word_count] >> 16;

        if (op == TG_SPIRV_OP_ENTRY_POINT)
        {
            const tg_spirv_execution_model execution_model = p_words[processed_word_count + 1];
            switch (execution_model)
            {
            case TG_SPIRV_EXECUTION_MODEL_VERTEX:     p_layout->shader_type = TG_SPIRV_SHADER_TYPE_VERTEX;   break;
            case TG_SPIRV_EXECUTION_MODEL_GEOMETRY:   p_layout->shader_type = TG_SPIRV_SHADER_TYPE_GEOMETRY; break;
            case TG_SPIRV_EXECUTION_MODEL_FRAGMENT:   p_layout->shader_type = TG_SPIRV_SHADER_TYPE_FRAGMENT; break;
            case TG_SPIRV_EXECUTION_MODEL_GL_COMPUTE: p_layout->shader_type = TG_SPIRV_SHADER_TYPE_COMPUTE;  break;

            default: TG_INVALID_CODEPATH(); break;
            }

            TG_ASSERT(tg_string_equal((char*)&p_words[processed_word_count + 3], "main"));

            break;
        }
        processed_word_count += op_word_count;
    }

    // COUNT WORDS

    processed_word_count = 5;
    u32 op_decorate_count = 0;
    u32 op_variable_count = 0;
    u32 max_op_referenceable_id = 0;
    while (processed_word_count < word_count)
    {
        const tg_spirv_op op = p_words[processed_word_count] & 0xffff;
        const u16 op_word_count = p_words[processed_word_count] >> 16;

        switch (op)
        {
        case TG_SPIRV_OP_TYPE_BOOL:
        case TG_SPIRV_OP_TYPE_INT:
        case TG_SPIRV_OP_TYPE_FLOAT:
        case TG_SPIRV_OP_TYPE_VECTOR:
        case TG_SPIRV_OP_TYPE_MATRIX:
        case TG_SPIRV_OP_TYPE_IMAGE:
        case TG_SPIRV_OP_TYPE_SAMPLER:
        case TG_SPIRV_OP_TYPE_SAMPLED_IMAGE:
        case TG_SPIRV_OP_TYPE_ARRAY:
        case TG_SPIRV_OP_TYPE_RUNTIME_ARRAY:
        case TG_SPIRV_OP_TYPE_STRUCT:
        case TG_SPIRV_OP_TYPE_OPAQUE:
        case TG_SPIRV_OP_TYPE_POINTER:
        case TG_SPIRV_OP_TYPE_FUNCTION:
        case TG_SPIRV_OP_TYPE_EVENT:
        case TG_SPIRV_OP_TYPE_DEVICE_EVENT:
        case TG_SPIRV_OP_TYPE_RESERVE_ID:
        case TG_SPIRV_OP_TYPE_QUEUE:
        case TG_SPIRV_OP_TYPE_PIPE:
        {
            const u32 id = p_words[processed_word_count + 1];
            max_op_referenceable_id = TG_MAX(id, max_op_referenceable_id);
        } break;
        case TG_SPIRV_OP_CONSTANT_TRUE:
        case TG_SPIRV_OP_CONSTANT_FALSE:
        case TG_SPIRV_OP_CONSTANT:
        case TG_SPIRV_OP_CONSTANT_COMPOSITE:
        case TG_SPIRV_OP_CONSTANT_SAMPLER:
        case TG_SPIRV_OP_SPEC_CONSTANT_TRUE:
        case TG_SPIRV_OP_SPEC_CONSTANT_FALSE:
        case TG_SPIRV_OP_SPEC_CONSTANT:
        case TG_SPIRV_OP_SPEC_CONSTANT_COMPOSITE:
        case TG_SPIRV_OP_SPEC_CONSTANT_OP:
        {
            const u32 id = p_words[processed_word_count + 2];
            max_op_referenceable_id = TG_MAX(id, max_op_referenceable_id);
        } break;
        case TG_SPIRV_OP_VARIABLE:
            op_variable_count++;
            break;
        case TG_SPIRV_OP_DECORATE:
            op_decorate_count++;
            break;

        default: break;
        }
        
        processed_word_count += op_word_count;
    }
    const u32 op_referencable_count = max_op_referenceable_id + 1;

    // MALLOC BUFFERS

    const tg_size op_decorate_buffer_size      = (tg_size)op_decorate_count     * sizeof(tg_spirv_op_decorate);
    const tg_size op_referenceable_buffer_size = (tg_size)op_referencable_count * sizeof(tg_spirv_op_referenceable); // Indexing starts at one, zero stays empty
    const tg_size op_variable_buffer_size      = (tg_size)op_variable_count     * sizeof(tg_spirv_op_variable);

    tg_spirv_op_decorate*      p_op_decorate_buffer      = TG_MALLOC_STACK(op_decorate_buffer_size);
    tg_spirv_op_referenceable* p_op_referenceable_buffer = TG_MALLOC_STACK(op_referenceable_buffer_size);
    tg_spirv_op_variable*      p_op_variable_buffer      = TG_MALLOC_STACK(op_variable_buffer_size);

    // Not every op is used, which is why 'op' of every unused slot should be 'TG_SPIRV_OP_NOP'
    for (u32 i = 0; i < op_referencable_count; i++)
    {
        p_op_referenceable_buffer[i].op = TG_SPIRV_OP_NOP;
    }

    // FILL BUFFERS

    processed_word_count = 5;
    u32 op_decorate_idx = 0;
    u32 op_variable_idx = 0;
    while (processed_word_count < word_count)
    {
        const tg_spirv_op op = p_words[processed_word_count] & 0xffff;
        const u16 op_word_count = p_words[processed_word_count] >> 16;

        switch (op)
        {
        case TG_SPIRV_OP_TYPE_INT:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
            p->type_int.width = p_words[processed_word_count + 2];
            p->type_int.signedness = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_TYPE_FLOAT:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
            p->type_float.width = p_words[processed_word_count + 2];
        } break;
        case TG_SPIRV_OP_TYPE_VECTOR:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
            p->type_vector.id_component_type = p_words[processed_word_count + 2];
            p->type_vector.component_count = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_TYPE_MATRIX:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
            p->type_matrix.id_column_type = p_words[processed_word_count + 2];
            p->type_matrix.column_count = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_TYPE_IMAGE:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
        } break;
        case TG_SPIRV_OP_TYPE_SAMPLED_IMAGE:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
        } break;
        case TG_SPIRV_OP_TYPE_ARRAY:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
            p->type_array.id_element_type = p_words[processed_word_count + 2];
            p->type_array.id_length = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_TYPE_POINTER:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(1);
            p->type_pointer.storage_class = p_words[processed_word_count + 2];
            p->type_pointer.id_return_type = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_CONSTANT:
        {
            tg_spirv_op_referenceable* p = TG_SPIRV_OP_REFERENCEABLE_INIT(2);
            p->constant.id_result_type = p_words[processed_word_count + 1];
            p->constant.value = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_VARIABLE:
        {
            TG_ASSERT(op_variable_idx < op_variable_count);
            tg_spirv_op_variable* p = &p_op_variable_buffer[op_variable_idx++];
            
            p->id_result_type = p_words[processed_word_count + 1];
            p->id = p_words[processed_word_count + 2];
            p->storage_class = p_words[processed_word_count + 3];
        } break;
        case TG_SPIRV_OP_DECORATE:
        {
            TG_ASSERT(op_decorate_idx < op_decorate_count);
            tg_spirv_op_decorate* p = &p_op_decorate_buffer[op_decorate_idx++];
            
            p->id_target = p_words[processed_word_count + 1];
            p->decoration = p_words[processed_word_count + 2];
            switch (p->decoration)
            {
            case TG_SPIRV_DECORATION_BLOCK:
            case TG_SPIRV_DECORATION_BUFFER_BLOCK:
            case TG_SPIRV_DECORATION_LOCATION:
            case TG_SPIRV_DECORATION_BINDING:
            case TG_SPIRV_DECORATION_DESCRIPTOR_SET:
                p->value = p_words[processed_word_count + 3];
                break;

            default: break;
            }
        } break;
        default: break;
        }

        processed_word_count += op_word_count;
    }

    // FILL LAYOUT

    for (u32 ivar = 0; ivar < op_variable_count; ivar++)
    {
        const tg_spirv_op_variable* p = &p_op_variable_buffer[ivar];
        TG_ASSERT(p->id_result_type < op_referencable_count);
        
        const b32 fill_global_resource        = p->storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM || p->storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT;
        const b32 fill_vertex_shader_input    = p->storage_class == TG_SPIRV_STORAGE_CLASS_INPUT && p_layout->shader_type == TG_SPIRV_SHADER_TYPE_VERTEX;
        const b32 fill_fragment_shader_output = p->storage_class == TG_SPIRV_STORAGE_CLASS_OUTPUT && p_layout->shader_type == TG_SPIRV_SHADER_TYPE_FRAGMENT;

        if (fill_global_resource)
        {
            tg_spirv_global_resource res = { 0 };

            // FILL TYPE AND PRN ARRAY LENGTH

            // The type of a global resources is always contained by a pointer
            const tg_spirv_op_referenceable* p_pointer = TG_SPIRV_OP_REFERENCEABLE_GET(p->id_result_type);
            TG_ASSERT(p_pointer->op == TG_SPIRV_OP_TYPE_POINTER);

            // The type can be found by browsing the decorations
            if (p_pointer->type_pointer.storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM)
            {
                for (u32 idec = 0; idec < op_decorate_count; idec++)
                {
                    const tg_spirv_op_decorate* p_dec = &p_op_decorate_buffer[idec];
                    if (p_dec->id_target == p_pointer->type_pointer.id_return_type)
                    {
                        if (p_op_decorate_buffer[idec].decoration == TG_SPIRV_DECORATION_BLOCK)
                        {
                            res.type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_UNIFORM_BUFFER;
                            break;
                        }
                        else
                        {
                            TG_ASSERT(p_op_decorate_buffer[idec].decoration == TG_SPIRV_DECORATION_BUFFER_BLOCK);

                            res.type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_BUFFER;
                            break;
                        }
                    }
                }
            }
            // The type can be found in the referenceable ops
            else
            {
                TG_ASSERT(p_pointer->type_pointer.storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT);
                const tg_spirv_op_referenceable* p_op_type = TG_SPIRV_OP_REFERENCEABLE_GET(p_pointer->type_pointer.id_return_type);

                switch (p_op_type->op)
                {
                case TG_SPIRV_OP_TYPE_IMAGE:
                    res.type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_IMAGE;
                    break;
                case TG_SPIRV_OP_TYPE_SAMPLED_IMAGE:
                    res.type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
                case TG_SPIRV_OP_TYPE_ARRAY:
                {
                    const tg_spirv_op_referenceable* p_op_element_type = TG_SPIRV_OP_REFERENCEABLE_GET(p_op_type->type_array.id_element_type);
                    if (p_op_element_type->op == TG_SPIRV_OP_TYPE_IMAGE)
                    {
                        res.type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_IMAGE;
                    }
                    else
                    {
                        TG_ASSERT(p_op_element_type->op == TG_SPIRV_OP_TYPE_SAMPLED_IMAGE);
                        res.type = TG_SPIRV_GLOBAL_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
                    }

                    const tg_spirv_op_referenceable* p_op_length = TG_SPIRV_OP_REFERENCEABLE_GET(p_op_type->type_array.id_length);
                    TG_ASSERT(p_op_length->op == TG_SPIRV_OP_CONSTANT);                                                         // Length must come from a constant instruction ...
                    TG_ASSERT(TG_SPIRV_OP_REFERENCEABLE_GET(p_op_length->constant.id_result_type)->op == TG_SPIRV_OP_TYPE_INT); // ... of an integer-type scalar ...
                    TG_ASSERT(p_op_length->constant.value >= 1);                                                                // ... whose value is at least 1.
                    res.array_element_count = p_op_length->constant.value;
                } break;

                default: TG_INVALID_CODEPATH(); break;
                }
            }
            TG_ASSERT(res.type != TG_SPIRV_GLOBAL_RESOURCE_TYPE_INVALID);

            // FILL DESCRIPTOR SET AND BINDING

            for (u32 idec = 0; idec < op_decorate_count; idec++)
            {
                const tg_spirv_op_decorate* p_dec = &p_op_decorate_buffer[idec];
                if (p_dec->id_target == p->id)
                {
                    if (p_dec->decoration == TG_SPIRV_DECORATION_BINDING)
                    {
                        res.binding = p_dec->value;
                    }
                    else if (p_dec->decoration == TG_SPIRV_DECORATION_DESCRIPTOR_SET)
                    {
                        res.descriptor_set = p_dec->value;
                    }
                }
            }

            TG_ASSERT(p_layout->global_resources.count < TG_MAX_SHADER_GLOBAL_RESOURCES);
            p_layout->global_resources.p_resources[p_layout->global_resources.count++] = res;
        }
        else if (fill_vertex_shader_input || fill_fragment_shader_output)
        {
            tg_spirv_inout_resource res = { 0 };

            // Location
            for (u32 idec = 0; idec < op_decorate_count; idec++)
            {
                const tg_spirv_op_decorate* p_dec = &p_op_decorate_buffer[idec];
                if (p_dec->id_target == p->id && p_dec->decoration == TG_SPIRV_DECORATION_LOCATION)
                {
                    res.location = p_dec->value;
                    break;
                }
            }

            // Format
            res.format = tg__resolve_inout_resource_format(op_decorate_count, p_op_decorate_buffer, op_referencable_count, p_op_referenceable_buffer, TG_SPIRV_OP_REFERENCEABLE_GET(p->id_result_type));

            if (fill_vertex_shader_input)
            {
                TG_ASSERT(p_layout->vertex_shader_input.count < TG_MAX_SHADER_INPUTS);
                p_layout->vertex_shader_input.p_resources[p_layout->vertex_shader_input.count++] = res;
            }
            else
            {
                TG_ASSERT(fill_fragment_shader_output);

                TG_ASSERT(p_layout->fragment_shader_output.count < TG_MAX_SHADER_OUTPUTS);
                p_layout->fragment_shader_output.p_resources[p_layout->fragment_shader_output.count++] = res;
            }
        }
    }

    // CLEAN UP

    TG_FREE_STACK(op_variable_buffer_size);
    TG_FREE_STACK(op_referenceable_buffer_size);
    TG_FREE_STACK(op_decorate_buffer_size);

    if (p_layout->shader_type == TG_SPIRV_SHADER_TYPE_VERTEX && p_layout->vertex_shader_input.count != 0)
    {
        TG_QSORT(p_layout->vertex_shader_input.count, p_layout->vertex_shader_input.p_resources, tg__compare_by_location, TG_NULL);
    }

#undef TG_SPIRV_OP_REFERENCEABLE_GET
#undef TG_SPIRV_OP_REFERENCEABLE_INIT
}
