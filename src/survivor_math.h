#pragma once

#define Pi           3.14159265359f
#define Radians(deg) ((deg) * (Pi) / 180.0f)
#define Degrees(rad) ((rad) * (180.0f) / (Pi))
#define Max(a, b)    ((a) > (b) ? (a) : (b))
#define Min(a, b)    ((a) < (b) ? (a) : (b))
#define Abs(a)       ((a >= 0.0f) ? (a) : -a)

inline glm::vec3 SafeNorm(const glm::vec3& v)
{
    f32 len = glm::length(v);
    if (len < 0.0001f)
    {
        return glm::vec3{ 0.0f };
    }
    return v / len;
}