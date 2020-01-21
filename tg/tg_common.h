#ifndef TG_COMMON_H
#define TG_COMMON_H

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#define TG_WINDOWS
#endif

#ifndef NDEBUG
#define ASSERT(x) if (!(x)) \
    {                       \
        *(int*)0 = 0;       \
    }
#else
#define ASSERT(x)
#endif

typedef unsigned int uint;

typedef int8_t       int8;
typedef int16_t      int16;
typedef int32_t      int32;
typedef int64_t      int64;

typedef uint8_t      uint8;
typedef uint16_t     uint16;
typedef uint32_t     uint32;
typedef uint64_t     uint64;

#endif
