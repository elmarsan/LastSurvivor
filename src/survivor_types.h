#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <vector>
#include <unordered_map>
#include <queue>

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

#define global_variable static
#define internal        static
#define local_persist   static

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
#define DefaultCase                                                                                                    \
    default:                                                                                                           \
    {                                                                                                                  \
        break;                                                                                                         \
    }

// https://www.redblobgames.com/pathfinding/a-star/implementation.html
template <typename T, typename priority_t>
struct PriorityQueue
{
    typedef std::pair<priority_t, T>                                                PQElement;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> elements;

    inline bool empty() const { return elements.empty(); }

    inline void put(T item, priority_t priority) { elements.emplace(priority, item); }

    T get()
    {
        T best_item = elements.top().second;
        elements.pop();
        return best_item;
    }
};