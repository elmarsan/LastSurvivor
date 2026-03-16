#pragma once

struct Camera
{
    v3  position;
    v3  target;
    v3  up;
    f32 pitch;
    f32 yaw;
    f32 fov;
    b32 topDownMode;
};

internal void _CameraUpdateVectors(Camera* camera)
{
    if (!camera->topDownMode)
    {
        camera->target.x = cosf(Radians(camera->yaw)) * cosf(Radians(camera->pitch));
        camera->target.y = sinf(Radians(camera->pitch));
        camera->target.z = sinf(Radians(camera->yaw)) * cosf(Radians(camera->pitch));
        camera->target   = Norm(camera->target);

        v3 worldUp = { 0.0f, 1.0f, 0.0f };
        v3 right   = Norm(Cross(camera->target, worldUp));
        v3 up      = Norm(Cross(right, camera->target));

        camera->up = up;
    }
}

inline void CameraInit(Camera* camera, v3 position, v3 target, v3 up, f32 pitch, f32 yaw, f32 fov)
{
    camera->position    = position;
    camera->target      = target;
    camera->pitch       = pitch;
    camera->yaw         = yaw;
    camera->fov         = fov;
    camera->topDownMode = false;

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

// TODO: Make topDown position configurable
inline v3 CameraPosition(Camera* camera) { return camera->topDownMode ? v3{ 0.0f, 40.0f, 0.0f } : camera->position; }

inline mat4x4 CameraView(Camera* camera)
{
    v3 target   = camera->target;
    v3 up       = camera->up;
    v3 position = CameraPosition(camera);
    if (camera->topDownMode)
    {
        v3 forward{ 0.0f, 0.0f, -1.0f };

        target = { 0.0f, -1.0f, 0.0f };
        up     = forward;
    }

    return LookAt(position, position + target, up);
}

inline void CameraToggleTopDownMode(Camera* camera) { camera->topDownMode = !camera->topDownMode; }