#ifndef TG_COMMON_H
#define TG_COMMON_H



#if defined(_WIN32) || defined(_WIN64)
#define TG_WIN32
#endif

#ifndef NDEBUG
#define TG_DEBUG
#else
//#define TG_DISTRIBUTE
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define TG_COMPILER_MSVC
#endif

#if defined(TG_COMPILER_MSVC) && _M_X64 == 100
#define TG_CPU_x64
#endif



#ifdef TG_WIN32
#define _CRT_SECURE_NO_WARNINGS
#define TG_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef TG_DEBUG
#define TG_ASSERT(x)                       if (!(x)) *(int*)0 = 0
#define TG_ASSERT2(x, p_message)           if (!(x)) { TG_DEBUG_LOG(p_message); *(int*)0 = 0; }
#define TG_DEBUG_PARAM(param)              , param
#define TG_INVALID_CODEPATH()              *(int*)0 = 0
#define TG_NOT_IMPLEMENTED()               *(int*)0 = 0
#else
#define TG_ASSERT(x)
#define TG_ASSERT2(x, p_message)
#define TG_DEBUG_PARAM(param)
#define TG_INVALID_CODEPATH()
#define TG_NOT_IMPLEMENTED()
#endif

#define TG_INOUT
#define TG_OUT



#define TG_NULL                            ((void*)0)

#define TG_TRUE                            1
#define TG_FALSE                           0

#define TG_F32_MAX                         3.402823466e+38f
#define TG_F32_MIN                         (-TG_F32_MAX)
#define TG_F32_MIN_POSITIVE                1.175494351e-38f
#define TG_F32_EPSILON                     1.192092896e-07f
#define TG_F64_MAX_POSITIVE                1.7976931348623158e+308
#define TG_F64_MIN_POSITIVE                2.2250738585072014e-308
#define TG_F64_EPSILON                     2.2204460492503131e-016

#define TG_I8_MIN                          (-127i8 - 1)
#define TG_I16_MIN                         (-32767i16 - 1)
#define TG_I32_MIN                         (-2147483647i32 - 1)
#define TG_I64_MIN                         (-9223372036854775807i64 - 1)
#define TG_I8_MAX                          127i8
#define TG_I16_MAX                         32767i16
#define TG_I32_MAX                         2147483647i32
#define TG_I64_MAX                         9223372036854775807i64

#define TG_U8_MAX                          0xffui8
#define TG_U16_MAX                         0xffffui16
#define TG_U32_MAX                         0xffffffffui32
#define TG_U64_MAX                         0xffffffffffffffffui64

#define TG_IEEE_754_FLOAT_SIGN_BITS        1
#define TG_IEEE_754_FLOAT_EXPONENT_BITS    8
#define TG_IEEE_754_FLOAT_MANTISSA_BITS    23

#define TG_IEEE_754_FLOAT_SIGN_MASK        0x80000000
#define TG_IEEE_754_FLOAT_EXPONENT_MASK    0x7f800000
#define TG_IEEE_754_FLOAT_MANSISSA_MASK    0x007fffff



typedef int                                b32;
typedef float                              f32;
#ifdef TG_CPU_x64
typedef double                             f64;
#endif
typedef signed char                        i8;
typedef short                              i16;
typedef int                                i32;
#ifdef TG_CPU_x64
typedef long long                          i64;
#endif
typedef unsigned char                      u8;
typedef unsigned short                     u16;
typedef unsigned int                       u32;
#ifdef TG_CPU_x64
typedef unsigned long long                 u64;
typedef unsigned long long                 tg_size;
#else
typedef unsigned long                      tg_size;
#endif

#endif
