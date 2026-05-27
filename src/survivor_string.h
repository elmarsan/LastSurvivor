#pragma once

inline size_t GetParentPathLength(char* path)
{
    char* last_slash = strrchr(path, '/');
    if (!last_slash)
    {
        return 0;
    }

    return (last_slash - path) + 1;
}

inline char* GetFilenameExtension(char* filename)
{
    char* dot = strrchr(filename, '.');
    if (!dot || dot == filename)
    {
        return "";
    }
    return dot + 1;
}

inline b32 StrEquals(char* a, char* b)
{
    while (*a && *b)
    {
        if (*a++ != *b++)
        {
            return false;
        }
    }
    return *a == *b;
}

inline b32 StrStartsWith(char* a, char* prefix)
{
    while (*prefix)
    {
        if (*a == 0 || *a != *prefix)
        {
            return false;
        }

        a++;
        prefix++;
    }

    return true;
}

inline char* StrCopy(Arena* arena, char* src)
{
    size_t size = 1 + strlen(src);
    char*  dst  = PushString(arena, size);
    memcpy(dst, src, size);
    return dst;
}
