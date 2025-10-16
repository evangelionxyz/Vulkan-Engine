// Copyright 2025, Evangelion Manuhutu

#ifndef CAMERA_HPP
#define CAMERA_HPP

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera
{
public:
    Camera(float fov = 45.0f, float width = 1280.0f, float height = 72.0f, float near_clip = 0.1f, float far_clip = 500.0f);

    void update_view_matrix();
    void update_projection_matrix();

    Camera &set_position(const glm::vec3 &position);
    Camera &set_fov(float degrees);
    Camera &set_near_clip(float near_clip);
    Camera &set_far_clip(float far_clip);
    Camera &resize(const glm::vec2 &size);

    const glm::mat4 get_view_projection_matrix();
    const glm::mat4 &get_view_matrix();
    const glm::mat4 &get_projection_matrix();

private:
    glm::vec3 m_Position;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjectionMatrix;

    glm::vec2 m_Size = {1.0f, 1.0f};
    float m_Fov = 45.0f;
    float m_NearClip = 0.1f;
    float m_FarClip = 500.0f;
};

#endif