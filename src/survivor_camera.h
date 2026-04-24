#pragma once

struct Camera
{
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    f32       pitch;
    f32       yaw;
    f32       fov;
};

internal void _CameraUpdateVectors(Camera* camera)
{

    camera->target.x = cosf(Radians(camera->yaw)) * cosf(Radians(camera->pitch));
    camera->target.y = sinf(Radians(camera->pitch));
    camera->target.z = sinf(Radians(camera->yaw)) * cosf(Radians(camera->pitch));
    camera->target   = SafeNorm(camera->target);

    glm::vec3 worldUp{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right = SafeNorm(glm::cross(camera->target, worldUp));
    glm::vec3 up    = SafeNorm(glm::cross(right, camera->target));

    camera->up = up;
}

inline void CameraInit(Camera* camera, glm::vec3 position, glm::vec3 target, glm::vec3 up, f32 pitch, f32 yaw, f32 fov)
{
    camera->position = position;
    camera->target   = target;
    camera->pitch    = pitch;
    camera->up       = up;
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
    f32       velocity = cameraSpeed * delta;
    glm::vec3 right    = SafeNorm(glm::cross(camera->target, camera->up));
    camera->position += right * velocity;
}

inline void CameraMoveRight(Camera* camera, f32 delta)
{
    f32       velocity = cameraSpeed * delta;
    glm::vec3 right    = SafeNorm(glm::cross(camera->target, camera->up));
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

inline glm::mat4 CameraGetView(Camera* camera) { return glm::lookAt(camera->position, camera->target, camera->up); }

inline glm::mat4 CameraGetProjection(Camera* camera, f32 aspectRatio)
{
    return glm::perspective(Radians(camera->fov), aspectRatio, 0.1f, 100.0f);
}