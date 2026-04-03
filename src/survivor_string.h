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