#ifndef CAMERA_HXX
#define CAMERA_HXX

#include "../external/math/math.hxx"
#include "ubo.hxx"

enum class CameraMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

/**
 * Camera — owns view and projection parameters and produces a CameraUBO
 * ready to upload to the GPU each frame.
 *
 * Kept deliberately thin: no Vulkan handles, no buffers.
 * The Descriptor class owns the GPU-side buffer; Camera just provides the data.
 */
class Camera{
public:

    /**
     * @param position   World-space eye position.
     * @param target     World-space point the camera looks at.
     * @param up         Up vector (usually {0, -1, 0} in Vulkan's Y-down NDC).
     * @param fovDeg     Vertical field of view in degrees.
     * @param aspect     Viewport width / height.
     * @param nearPlane  Near clip distance (keep as large as practical).
     * @param farPlane   Far clip distance.
     */
    Camera(
        Vector3f position    = { 0.0f,  0.0f,  3.0f},
        float    yaw         = -90.0f,   // looking toward -Z initially
        float    pitch       = 0.0f,
        Vector3f worldUp     = { 0.0f,  -1.0f,  0.0f},
        float    fovDeg      = 45.0f,
        float    aspect      = 800.0f / 600.0f,
        float    nearPlane   = 0.1f,
        float    farPlane    = 100.0f,
        float    moveSpeed   = 5.0f,
        float    sensitivity = 0.1f);

    /**
     * Move the camera based on a direction and how much time has passed.
     * Multiplying by deltaTime makes movement frame-rate independent.
     */
    void processKeyboard(CameraMovement direction, float deltaTime);

    /**
     * Rotate the camera based on mouse delta (pixels moved since last frame).
     * @param constrainPitch  Clamp pitch to [-89, 89] to avoid gimbal lock.
     */
    void processMouse(float dx, float dy, bool constrainPitch = true);

    // ---- Produce GPU data --------------------------------------------------
    CameraUBO getUBO() const;

    // ---- Mutators ----------------------------------------------------------
    void setAspect(float aspect) { m_aspect = aspect; }
    void setFov(float fovDeg)    { m_fovDeg = fovDeg; }
    void setPosition(Vector3f p) { m_position = p;    }

    // ---- Accessors ---------------------------------------------------------
    Vector3f getPosition() const { return m_position; }
    Vector3f getFront()    const { return m_front;    }
    float    getFov()      const { return m_fovDeg;   }

private:
    // World-space state
    Vector3f m_position;
    Vector3f m_worldUp;

    // Euler angles (degrees)
    float m_yaw;
    float m_pitch;

    // Derived vectors -- rebuilt by updateVectors() after any angle change
    Vector3f m_front = { 0.0f, 0.0f, -1.0f };
    Vector3f m_right = { 1.0f, 0.0f,  0.0f };
    Vector3f m_up    = { 0.0f, 1.0f,  0.0f };

    // Projection parameters
    float m_fovDeg;
    float m_aspect;
    float m_nearPlane;
    float m_farPlane;

    // Input sensitivity
    float m_moveSpeed;
    float m_sensitivity;

    // Recomputes m_front, m_right, m_up from m_yaw and m_pitch.
    void updateVectors();

};

#endif