#include "../headers/camera.hxx"

Camera::Camera(
    Vector3f position,
    Vector3f target,
    Vector3f up,
    float    fovDeg,
    float    aspect,
    float    nearPlane,
    float    farPlane)
    : m_position(position),
      m_target(target),
      m_up(up),
      m_fovDeg(fovDeg),
      m_aspect(aspect),
      m_nearPlane(nearPlane),
      m_farPlane(farPlane)
{
}

void Camera::setPosition(Vector3f position) { m_position = position; }
void Camera::setTarget(Vector3f target)     { m_target   = target;   }
void Camera::setAspect(float aspect)        { m_aspect   = aspect;   }
void Camera::setFov(float fovDeg)           { m_fovDeg   = fovDeg;   }

CameraUBO Camera::getUBO() const
{
    CameraUBO ubo{};

    ubo.view = look_at(m_position, m_target, m_up);

    // glm::perspective produces a matrix for OpenGL's clip space where Z runs
    // from -1 to 1.  Vulkan uses 0 to 1, so we flip the sign of element [1][1]
    // to correct the Y-axis inversion that would otherwise flip geometry.
    ubo.proj = perspective(
        m_fovDeg,
        m_aspect,
        m_nearPlane,
        m_farPlane);

    // GLM was designed for OpenGL, where Y in clip space is flipped relative
    // to Vulkan.  Negating proj[1][1] corrects for this without needing a
    // separate compile-time define (GLM_FORCE_DEPTH_ZERO_TO_ONE etc.).
    ubo.proj.yy *= -1.0f;

    return ubo;
}