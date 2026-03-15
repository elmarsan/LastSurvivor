#pragma once

struct Camera
{
    v3  position;
    v3  target;
    v3  up;
    f32 pitch;
    f32 yaw;
    f32 fov;
};

internal void _CameraUpdateVectors(Camera* camera)
{
    camera->target.x = cosf(Radians(camera->yaw)) * cosf(Radians(camera->pitch));
    camera->target.y = sinf(Radians(camera->pitch));
    camera->target.z = sinf(Radians(camera->yaw)) * cosf(Radians(camera->pitch));
    camera->target   = Norm(camera->target);

    v3 worldUp = { 0.0f, 1.0f, 0.0f };
    v3 right   = Norm(Cross(camera->target, worldUp));
    v3 up      = Norm(Cross(right, camera->target));

    camera->up = up;

    camera->target = { 0.0f, -1.0f, 0.0f };
    v3 forward     = { 0.0f, 0.0f, -1.0f };
    camera->up     = forward;
}

inline void CameraInit(Camera* camera, v3 position, v3 target, v3 up, f32 pitch, f32 yaw, f32 fov)
{
    camera->position = position;
    camera->target   = target;
    camera->pitch    = pitch;
    camera->yaw      = yaw;
    camera->fov      = fov;

    _CameraUpdateVectors(camera);
}

internal f32 cameraSpeed = 1.8f;

inline void CameraMoveForward(Camera* camera, f32 delta)
{
    f32 velocity = cameraSpeed * delta;

    camera->position += camera->target * velocity;
}

inline void CameraMoveBackward(Camera* camera, f32 delta)
{
    f32 velocity = cameraSpeed * delta;

    camera->position -= camera->target * velocity;
}

inline void CameraMoveLeft(Camera* camera, f32 delta)
{
    f32 velocity = cameraSpeed * delta;
    v3  right    = Norm(Cross(camera->target, camera->up));
    camera->position += right * velocity;
}

inline void CameraMoveRight(Camera* camera, f32 delta)
{
    f32 velocity = cameraSpeed * delta;
    v3  right    = Norm(Cross(camera->target, camera->up));
    camera->position -= right * velocity;
}

inline void CameraSetPitch(Camera* camera, f32 pitch)
{
    camera->pitch = pitch;
    _CameraUpdateVectors(camera);
}

inline void CameraSetYaw(Camera* camera, f32 yaw)
{
    camera->yaw = yaw;
    _CameraUpdateVectors(camera);
}

inline mat4x4 CameraView(Camera* camera)
{
    return LookAt(camera->position, camera->position + camera->target, camera->up);
}