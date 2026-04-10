#pragma once

inline size_t GetParentPathLength(const char* path)
{
    const char* last_slash = strrchr(path, '/');
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