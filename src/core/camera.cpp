// Copyright 2025, Evangelion Manuhutu

#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float fov, float width, float height, float near_clip, float far_clip)
    : m_Position(0.0f), m_Fov(fov), m_NearClip(near_clip), m_FarClip(far_clip), m_Size({width, height})
{
    const float aspect = width / height;
    m_ProjectionMatrix = glm::perspectiveZO(glm::radians(fov), aspect, near_clip, far_clip);
}

void Camera::update_view_matrix()
{
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position);
    m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

void Camera::update_projection_matrix()
{
    const float aspect = m_Size.x / m_Size.y;
    m_ProjectionMatrix = glm::perspectiveZO(glm::radians(m_Fov), aspect, m_NearClip, m_FarClip);
}

Camera &Camera::set_position(const glm::vec3 &position)
{
    m_Position = position;
    return *this;
}

Camera &Camera::set_fov(float degrees)
{
    m_Fov = degrees;
    return *this;
}

Camera &Camera::set_near_clip(float near_clip)
{
    m_NearClip = near_clip;
    return *this;
}

Camera &Camera::set_far_clip(float far_clip)
{
    m_FarClip = far_clip;
    return *this;
}

Camera &Camera::resize(const glm::vec2 &size)
{
    m_Size = size;
    return *this;
}

const glm::mat4 Camera::get_view_projection_matrix()
{
    return m_ProjectionMatrix * m_ViewMatrix;
}

const glm::mat4 &Camera::get_view_matrix()
{
    return m_ViewMatrix;
}

const glm::mat4 &Camera::get_projection_matrix()
{
    return m_ProjectionMatrix;
}
