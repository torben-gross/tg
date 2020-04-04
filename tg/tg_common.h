#ifndef TG_COMMON_H
#define TG_COMMON_H



#if defined(_WIN32) || defined(_WIN64)
#define                       TG_WIN32
#endif

#ifdef TG_WIN32
#define                       _CRT_SECURE_NO_WARNINGS
#define                       TG_VULKAN
#define                       VK_USE_PLATFORM_WIN32_KHR
#endif

#if _MSC_VER && !__INTEL_COMPILER
#define                       TG_COMPILER_MSVC
#endif



#ifndef NDEBUG
#define TG_DEBUG
#endif

#ifdef TG_DEBUG
#define TG_ASSERT(x)          if (!(x)) *(int*)0 = 0
#else
#define TG_ASSERT(x)
#endif



#define NULL                  0

#define TG_TRUE               1
#define TG_FALSE              0

#define F32_MIN_POSITIVE      1.175494351e-38f
#define F32_MAX_POSITIVE      3.402823466e+38f
#define F32_EPSILON           1.192092896e-07f
#define F64_MIN_POSITIVE      2.2250738585072014e-308
#define F64_MAX_POSITIVE      1.7976931348623158e+308
#define F64_EPSILON           2.2204460492503131e-016

#define I8_MIN                (-127i8 - 1)
#define I16_MIN               (-32767i16 - 1)
#define I32_MIN               (-2147483647i32 - 1)
#define I64_MIN               (-9223372036854775807i64 - 1)
#define I8_MAX                127i8
#define I16_MAX               32767i16
#define I32_MAX               2147483647i32
#define I64_MAX               9223372036854775807i64

#define UI8_MAX               0xffui8
#define UI16_MAX              0xffffui16
#define UI32_MAX              0xffffffffui32
#define UI64_MAX              0xffffffffffffffffui64



typedef int                   b32;

typedef float                 f32;
typedef double                f64;

typedef signed char           i8;
typedef short                 i16;
typedef int                   i32;
typedef long long             i64;

typedef unsigned int          ui;
typedef unsigned char         ui8;
typedef unsigned short        ui16;
typedef unsigned int          ui32;
typedef unsigned long long    ui64;

#endif
