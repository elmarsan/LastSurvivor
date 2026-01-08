#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

#define global        static
#define internal      static
#define local_persist static

#ifdef BUILD_TYPE_DEBUG
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

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase                                                                                             \
    default:                                                                                                           \
    {                                                                                                                  \
        InvalidCodePath;                                                                                               \
        break;                                                                                                         \
    }

union v2
{
    f32 e[2];
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 w, h;
    };
};

union v2u
{
    u32 e[2];
    struct
    {
        u32 x, y;
    };
    struct
    {
        u32 w, h;
    };
};

union v2s
{
    s32 e[2];
    struct
    {
        s32 x, y;
    };
    struct
    {
        s32 w, h;
    };
};

union mat3x3
{
    struct
    {
        f32 e[3][3];
    };
    struct
    {
        f32 ptr[12];
    };

    mat3x3(f32 e00, f32 e01, f32 e02, f32 e10, f32 e11, f32 e12, f32 e20, f32 e21, f32 e22)
    {
        e[0][0] = e00;
        e[0][1] = e01;
        e[0][2] = e02;

        e[1][0] = e10;
        e[1][1] = e11;
        e[1][2] = e12;

        e[2][0] = e20;
        e[2][1] = e21;
        e[2][2] = e22;
    }
};

union mat4x4
{
    struct
    {
        f32 e[4][4];
    };
    struct
    {
        f32 ptr[16];
    };
};

union v3
{
    f32 e[3];
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 r, g, b;
    };
};

union v4
{
    f32 e[4];
    struct
    {
        f32 x, y, z, w;
    };
    struct
    {
        f32 r, g, b, a;
    };
};