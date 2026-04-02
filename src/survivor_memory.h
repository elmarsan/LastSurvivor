#pragma once

struct Arena
{
    u8*    base;
    u8*    ptr;
    size_t size;
    size_t used;
};

#define PushStruct(arena, type)       (type*)_ArenaPush(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)_ArenaPush(arena, (count) * sizeof(type))
#define PushBlock(arena, size)        _ArenaPush(arena, size)

inline void ArenaInit(Arena* arena, size_t size, void* buffer)
{
    arena->base = (u8*)buffer;
    arena->ptr  = (u8*)buffer;
    arena->size = size;
    arena->used = 0;
}

inline size_t ArenaGetAlignmentOffset(Arena* arena, size_t alignment)
{
    size_t alignmentOffset = 0;
    size_t pointer         = (size_t)arena->ptr + arena->used;
    size_t alignmentMask   = alignment - 1;

    if (pointer & alignmentMask)
    {
        alignmentOffset = alignment - (pointer & alignmentMask);
    }

    return alignmentOffset;
}

inline void* _ArenaPush(Arena* arena, size_t size, size_t alignment = 4)
{
    size_t allocSize = size;

    size_t alignmentOffset = ArenaGetAlignmentOffset(arena, alignment);
    allocSize += alignmentOffset;

    Assert((arena->used + allocSize) <= arena->size);
    void* result = arena->ptr + alignmentOffset;
    arena->used += allocSize;
    arena->ptr += allocSize;
    Assert(allocSize >= size);

    return result;
}

inline void ArenaClear(Arena* arena)
{
    arena->used = 0;
    arena->ptr  = arena->base;
}

struct Stream
{
    u8* beginBlock;
    u8* ptr;
};

inline Stream StreamInit(u8* data) { return Stream{ data, data }; }

inline void StreamRead(Stream* stream, void* dst, size_t size, size_t count)
{
    memcpy(dst, stream->ptr, size * count);
    stream->ptr += size * count;
}

inline void StreamSkip(Stream* stream, size_t offset) { stream->ptr += offset; }