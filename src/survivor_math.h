#pragma once

#define Pi           3.14159265359f
#define Radians(deg) ((deg) * (Pi) / 180.0f)

inline v3 operator-(v3& a)
{
    v3 result;

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

inline v3 operator-(v3 a, v3 b)
{
    v3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

inline v3& operator-=(v3& a, v3& b)
{
    a = a - b;
    return a;
}

inline v3 operator+(v3 a, v3 b)
{
    v3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

inline v3& operator+=(v3& a, v3& b)
{
    a = a + b;
    return a;
}

inline v3 operator*(v3 a, f32 scalar)
{
    v3 result;

    result.x = a.x * scalar;
    result.y = a.y * scalar;
    result.z = a.z * scalar;

    return result;
}

inline f32 Length(const v3& a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }

inline v3 Norm(v3 a)
{
    v3 result{ 0 };

    f32 length = Length(a);
    if (length)
    {
        result.x = a.x / length;
        result.y = a.y / length;
        result.z = a.z / length;
    }

    return result;
}

inline v3 Cross(v3 a, v3 b)
{
    v3 result;

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}

inline f32 Dot(v3 a, v3 b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }

inline mat4x4 operator*(mat4x4 a, mat4x4 b)
{
    mat4x4 result = { 0 };

    // Column 0
    result.e[0][0] = a.e[0][0] * b.e[0][0] + a.e[1][0] * b.e[0][1] + a.e[2][0] * b.e[0][2] + a.e[3][0] * b.e[0][3];
    result.e[0][1] = a.e[0][1] * b.e[0][0] + a.e[1][1] * b.e[0][1] + a.e[2][1] * b.e[0][2] + a.e[3][1] * b.e[0][3];
    result.e[0][2] = a.e[0][2] * b.e[0][0] + a.e[1][2] * b.e[0][1] + a.e[2][2] * b.e[0][2] + a.e[3][2] * b.e[0][3];
    result.e[0][3] = a.e[0][3] * b.e[0][0] + a.e[1][3] * b.e[0][1] + a.e[2][3] * b.e[0][2] + a.e[3][3] * b.e[0][3];

    // Column 1
    result.e[1][0] = a.e[0][0] * b.e[1][0] + a.e[1][0] * b.e[1][1] + a.e[2][0] * b.e[1][2] + a.e[3][0] * b.e[1][3];
    result.e[1][1] = a.e[0][1] * b.e[1][0] + a.e[1][1] * b.e[1][1] + a.e[2][1] * b.e[1][2] + a.e[3][1] * b.e[1][3];
    result.e[1][2] = a.e[0][2] * b.e[1][0] + a.e[1][2] * b.e[1][1] + a.e[2][2] * b.e[1][2] + a.e[3][2] * b.e[1][3];
    result.e[1][3] = a.e[0][3] * b.e[1][0] + a.e[1][3] * b.e[1][1] + a.e[2][3] * b.e[1][2] + a.e[3][3] * b.e[1][3];

    // Column 2
    result.e[2][0] = a.e[0][0] * b.e[2][0] + a.e[1][0] * b.e[2][1] + a.e[2][0] * b.e[2][2] + a.e[3][0] * b.e[2][3];
    result.e[2][1] = a.e[0][1] * b.e[2][0] + a.e[1][1] * b.e[2][1] + a.e[2][1] * b.e[2][2] + a.e[3][1] * b.e[2][3];
    result.e[2][2] = a.e[0][2] * b.e[2][0] + a.e[1][2] * b.e[2][1] + a.e[2][2] * b.e[2][2] + a.e[3][2] * b.e[2][3];
    result.e[2][3] = a.e[0][3] * b.e[2][0] + a.e[1][3] * b.e[2][1] + a.e[2][3] * b.e[2][2] + a.e[3][3] * b.e[2][3];

    // Column 3
    result.e[3][0] = a.e[0][0] * b.e[3][0] + a.e[1][0] * b.e[3][1] + a.e[2][0] * b.e[3][2] + a.e[3][0] * b.e[3][3];
    result.e[3][1] = a.e[0][1] * b.e[3][0] + a.e[1][1] * b.e[3][1] + a.e[2][1] * b.e[3][2] + a.e[3][1] * b.e[3][3];
    result.e[3][2] = a.e[0][2] * b.e[3][0] + a.e[1][2] * b.e[3][1] + a.e[2][2] * b.e[3][2] + a.e[3][2] * b.e[3][3];
    result.e[3][3] = a.e[0][3] * b.e[3][0] + a.e[1][3] * b.e[3][1] + a.e[2][3] * b.e[3][2] + a.e[3][3] * b.e[3][3];

    return result;
}

inline v4 operator*(mat4x4 mat, v4 vec)
{
    v4 result{};

    result.x = mat.e[0][0] * vec.x + mat.e[1][0] * vec.y + mat.e[2][0] * vec.z + mat.e[3][0] * vec.w;
    result.y = mat.e[0][1] * vec.x + mat.e[1][1] * vec.y + mat.e[2][1] * vec.z + mat.e[3][1] * vec.w;
    result.z = mat.e[0][2] * vec.x + mat.e[1][2] * vec.y + mat.e[2][2] * vec.z + mat.e[3][2] * vec.w;
    result.w = mat.e[0][3] * vec.x + mat.e[1][3] * vec.y + mat.e[2][3] * vec.z + mat.e[3][3] * vec.w;

    return result;
}

inline mat4x4 Identity()
{
    mat4x4 result = { 0 };

    result.e[0][0] = 1.0f;
    result.e[1][1] = 1.0f;
    result.e[2][2] = 1.0f;
    result.e[3][3] = 1.0f;

    return result;
}

inline mat4x4 Translate(mat4x4 mat, v3 translate)
{
    mat4x4 result = mat;

    result.e[3][0] += translate.x;
    result.e[3][1] += translate.y;
    result.e[3][2] += translate.z;

    return result;
}

inline mat4x4 Rotate(mat4x4 mat, f32 angle, v3 rot)
{
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    v3 norm = Norm(rot);

    f32 x = norm.x;
    f32 y = norm.y;
    f32 z = norm.z;

    mat4x4 rotation = { 0 };

    rotation.e[0][0] = (1 - c) * (x * x) + c;
    rotation.e[0][1] = (1 - c) * x * y + s * z;
    rotation.e[0][2] = (1 - c) * x * z - s * y;

    rotation.e[1][0] = (1 - c) * x * y - s * z;
    rotation.e[1][1] = (1 - c) * (y * y) + c;
    rotation.e[1][2] = (1 - c) * y * z + s * x;

    rotation.e[2][0] = (1 - c) * x * z + s * y;
    rotation.e[2][1] = (1 - c) * y * z - s * x;
    rotation.e[2][2] = (1 - c) * (z * z) + c;

    rotation.e[3][3] = 1.0f;

    return mat * rotation;
}

inline mat4x4 Scale(mat4x4 mat4, v3 scale)
{
    mat4x4 scaleMat = Identity();

    scaleMat.e[0][0] *= scale.x;
    scaleMat.e[1][1] *= scale.y;
    scaleMat.e[2][2] *= scale.z;

    return mat4 * scaleMat;
}

inline mat4x4 Scale(mat4x4 mat4, f32 scale)
{
    mat4x4 scaleMat = Identity();

    scaleMat.e[0][0] *= scale;
    scaleMat.e[1][1] *= scale;
    scaleMat.e[2][2] *= scale;

    return mat4 * scaleMat;
}

inline mat4x4 Perspective(f32 fov, f32 aspect, f32 zNear, f32 zFar)
{
    mat4x4 result = { 0 };

    f32 cotan = 1.0f / tanf(fov / 2.0f);

    result.e[0][0] = cotan / aspect;
    result.e[1][1] = cotan;
    result.e[2][2] = (zNear + zFar) / (zNear - zFar);
    result.e[2][3] = -1.0f;
    result.e[3][2] = (2.0f * zNear * zFar) / (zNear - zFar);

    return result;
}

inline mat4x4 Orthographic(f32 left, f32 right, f32 bottom, f32 top)
{
    mat4x4 result = Identity();

    result.e[0][0] = 2.0f / (right - left);
    result.e[1][1] = 2.0f / (top - bottom);
    result.e[2][2] = -1.0f;
    result.e[3][0] = -(right + left) / (right - left);
    result.e[3][1] = -(top + bottom) / (top - bottom);

    return result;
}

inline mat4x4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar)
{
    mat4x4 result = Identity();

    result.e[0][0] = 2.0f / (right - left);
    result.e[1][1] = 2.0f / (top - bottom);
    result.e[2][2] = -2.0f / (zFar - zNear);
    result.e[3][0] = -(right + left) / (right - left);
    result.e[3][1] = -(top + bottom) / (top - bottom);
    result.e[3][2] = -(zFar + zNear) / (zFar - zNear);

    return result;
}

inline mat4x4 LookAt(v3 eye, v3 center, v3 worldUp)
{
    v3 forward = Norm(center - eye);
    v3 side    = Norm(Cross(forward, worldUp));
    v3 up      = Cross(side, forward);

    mat4x4 result = { 0 };

    result.e[0][0] = side.x;
    result.e[1][0] = side.y;
    result.e[2][0] = side.z;

    result.e[0][1] = up.x;
    result.e[1][1] = up.y;
    result.e[2][1] = up.z;

    result.e[0][2] = -forward.x;
    result.e[1][2] = -forward.y;
    result.e[2][2] = -forward.z;

    result.e[3][0] = -Dot(side, eye);
    result.e[3][1] = -Dot(up, eye);
    result.e[3][2] = Dot(forward, eye);
    result.e[3][3] = 1.0f;

    return result;
}