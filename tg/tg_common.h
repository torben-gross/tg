#ifndef TG_COMMON_H
#define TG_COMMON_H

#if defined(_WIN32) || defined(_WIN64)
#define TG_WIN32
#endif

#ifndef NDEBUG
#define TG_DEBUG
#endif

#ifdef TG_DEBUG
#define ASSERT(x) if (!(x)) *(int*)0 = 0
#else
#define ASSERT(x)
#endif

#include <stdint.h>

typedef float         float32;
typedef double        float64;

typedef char          int4;
typedef int8_t        int8;
typedef int16_t       int16;
typedef int32_t       int32;
typedef int64_t       int64;

typedef unsigned int  uint;
typedef unsigned char uint4;
typedef uint8_t       uint8;
typedef uint16_t      uint16;
typedef uint32_t      uint32;
typedef uint64_t      uint64;

#endif
