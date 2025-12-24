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
typedef float    f32;
typedef double   f64;

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

#define Log(fmt, ...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        printf(fmt, __VA_ARGS__);                                                                                      \
        printf("\n");                                                                                                  \
    } while (0)
#else
#define Assert(expr)
#define Log(...)
#endif

#define ArrayCount(arr) sizeof(arr) / sizeof(arr[0])

union v2
{
    struct
    {
        f32 x, y;
    };

    struct
    {
        f32 w, h;
    };

    f32 e[2];
};

union v2u
{
    struct
    {
        u32 x, y;
    };

    struct
    {
        u32 w, h;
    };

    u32 e[2];
};
