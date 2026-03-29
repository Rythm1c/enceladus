#ifndef CAMERA_HXX
#define CAMERA_HXX

#include "../external/math/math.hxx"
#include "ubo.hxx"

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
        Vector3f  position  = {0.0f, 0.0f,  3.0f},
        Vector3f  target    = {0.0f, 0.0f,  0.0f},
        Vector3f  up        = {0.0f, -1.0f, 0.0f},  // Vulkan Y-axis points down
        float     fovDeg    = 45.0f,
        float     aspect    = 800.0f / 600.0f,
        float     nearPlane = 0.1f,
        float     farPlane  = 100.0f
    );

    void setPosition(Vector3f position);
    void setTarget(Vector3f target);
    void setAspect(float aspect);          // call on window resize
    void setFov(float fovDeg);

    // ---- Produce GPU data ---------------------------------------------------
    /**
     * Builds and returns the CameraUBO for this frame.
     * Cheap — just two matrix multiplications.
     */
    CameraUBO getUBO() const;

    // ---- Accessors ----------------------------------------------------------
    Vector3f getPosition() const { return m_position; }
    Vector3f getTarget()   const { return m_target;   }

private:
    Vector3f m_position;
    Vector3f m_target;
    Vector3f m_up;
    float    m_fovDeg;
    float    m_aspect;
    float    m_nearPlane;
    float    m_farPlane;

};

#endif