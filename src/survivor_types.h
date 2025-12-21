#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef int32_t  b32;

#define global   static
#define internal static

#ifdef BUILD_TYPE_DEBUG
#include <stdio.h>
#define Assert(expr)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            printf("Assertion failed, file %s, line %d\n", __FILE__, __LINE__);                                        \
            __debugbreak();                                                                                            \
        }                                                                                                              \
    } while (0)

#else
#define Assert(expr)
#endif