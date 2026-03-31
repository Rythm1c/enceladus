#include "headers/camera.hxx"
#include <algorithm>
#include <cmath>

Camera::Camera(
    Vector3f position,
    float    yaw,
    float    pitch,
    Vector3f worldUp,
    float    fovDeg,
    float    aspect,
    float    nearPlane,
    float    farPlane,
    float    moveSpeed,
    float    sensitivity)
    : m_position(position),
      m_worldUp(worldUp),
      m_yaw(yaw),
      m_pitch(pitch),
      m_fovDeg(fovDeg),
      m_aspect(aspect),
      m_nearPlane(nearPlane),
      m_farPlane(farPlane),
      m_moveSpeed(moveSpeed),
      m_sensitivity(sensitivity)
{
    // Build the initial front/right/up from the starting yaw and pitch.
    updateVectors();
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime)
{
    const float velocity = m_moveSpeed * deltaTime;

    switch (direction)
    {
    case CameraMovement::Forward:  m_position += m_front * velocity;  break;
    case CameraMovement::Backward: m_position -= m_front * velocity;  break;
    // Strafe along the right vector (perpendicular to front in the XZ plane)
    case CameraMovement::Left:     m_position -= m_right * velocity;  break;
    case CameraMovement::Right:    m_position += m_right * velocity;  break;
    // Move along world Y so vertical movement stays absolute (not camera-relative)
    case CameraMovement::Up:       m_position += m_worldUp * velocity; break;
    case CameraMovement::Down:     m_position -= m_worldUp * velocity; break;
    }
}

void Camera::processMouse(float dx, float dy, bool constrainPitch)
{
    m_yaw   += dx * m_sensitivity;
    // Subtract dy because SDL gives positive dy when mouse moves *down*,
    // but a downward mouse movement should decrease pitch (look down).
    m_pitch -= dy * m_sensitivity;

    if (constrainPitch)
    {
        // Clamp pitch to just under 90 degrees in both directions.
        // At exactly +-90 the cross product used in updateVectors() degenerates
        // and the camera flips, which is the "gimbal lock" symptom.
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    updateVectors();
}

// =============================================================================
// GPU data
// =============================================================================

CameraUBO Camera::getUBO() const
{
    CameraUBO ubo{};

    // transpose() converts from row-major (math convention) to column-major (Vulkan convention).


    // lookAt(eye, target, up) -- target is a point in front of the camera
    // derived from position + front so the camera looks in the right direction
    // regardless of where it is in the world.
    ubo.view = look_at(m_position, m_position + m_front, m_up).transpose();

    ubo.proj = perspective(
        m_fovDeg,
        m_aspect,
        m_nearPlane,
        m_farPlane).transpose();

    // Flip Y: GLM uses OpenGL convention (Y up in NDC),
    // Vulkan uses Y down. Negating [1][1] corrects this.
    ubo.proj.yy *= -1.0f;

    return ubo;
}

// =============================================================================
// Private
// =============================================================================

void Camera::updateVectors()
{
    // Convert yaw and pitch (degrees) to a direction vector.
    // This is the standard spherical-to-Cartesian conversion for a
    // camera where yaw rotates around Y and pitch rotates around X.
    Vector3f front;
    front.x = std::cos(to_radians(m_yaw)) * std::cos(to_radians(m_pitch));
    front.y = std::sin(to_radians(m_pitch));
    front.z = std::sin(to_radians(m_yaw)) * std::cos(to_radians(m_pitch));

    m_front = front.unit();

    // Right vector: perpendicular to front and world up.
    // normalize() needed because the cross product length depends on the angle
    // between the two vectors.
    m_right = cross(m_front, m_worldUp).unit();

    // Camera-local up: perpendicular to both front and right.
    // This tilts with the camera when pitch changes.
    m_up = cross(m_right, m_front).unit();
}
