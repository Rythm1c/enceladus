#ifndef SHAPE_HXX
#define SHAPE_HXX

#include <vector>
#include <vulkan/vulkan.h>

#include "buffer.hxx"
#include "vertex.hxx"

#include "../external/math/vec2.hxx"
#include "../external/math/vec3.hxx"
#include "../external/math/mat4.hxx"
#include "../external/math/transform.hxx"

class Core;

// =============================================================================
// Push constants — 2D offset only for now; expand to mat4 once depth is ready
// =============================================================================
struct ModelPushConstants
{
    Mat4x4 model = Mat4x4::identity();
};

// =============================================================================
// Shape — non-owning base class for all renderable primitives
// =============================================================================
class Shape
{
public:
    explicit Shape(Core &core, Vector2f position = {0.0f, 0.0f});

    Shape(const Shape &)            = delete;
    Shape &operator=(const Shape &) = delete;

    // Movable so shapes can live in vectors
    Shape(Shape &&)            = default;
    Shape &operator=(Shape &&) = default;

    virtual ~Shape() = default;

    // Call once after construction to generate geometry and upload to the GPU.
    void upload();

    /**
     * Records all draw commands into @p cmd.
     * Called by Renderer::drawShape() — do not call directly.
     *
     * @param cmd    The active command buffer (inside a render pass).
     * @param layout The pipeline layout (needed for push constants).
     */
    void draw(VkCommandBuffer cmd, VkPipelineLayout layout) const;

    // ---- Transform ----------------------------------------------------------
    void setPosition(Vector3f translation);
    void setRotation(float angleDeg, Vector3f axis);
    void setScale(Vector3f s);

    Mat4x4 getModel() const { return model.toMat4x4(); }

protected:
    Core                  &m_core;
    Buffer                m_vertexBuffer;
    Buffer                m_indexBuffer;
    Transform             model;
    std::vector<Vertex>   m_vertices;  // filled by buildGeometry()
    std::vector<uint16_t> m_indices;

    /**
     * Derived classes override this to populate m_vertices.
     * Called automatically by upload().
     */
    virtual void buildGeometry() = 0;
};

// =============================================================================
// Triangle
// =============================================================================
class Triangle : public Shape
{
public:
    /**
     * @param core     Core reference.
     * @param position Centre of the triangle in NDC space.
     * @param size     Half-extent of the triangle (NDC units).
     * @param colors   Per-vertex colours (RGB). Defaults to red/green/blue.
     */
    Triangle(
        Core            &core,
        Vector2f         position = {0.0f, 0.0f},
        float            size     = 0.5f,
        Vector3f         colorA   = {1.0f, 0.0f, 0.0f},
        Vector3f         colorB   = {0.0f, 1.0f, 0.0f},
        Vector3f         colorC   = {0.0f, 0.0f, 1.0f});

private:
    float    m_size;
    Vector3f m_colorA;
    Vector3f m_colorB;
    Vector3f m_colorC;

    void buildGeometry() override;
};

#endif // SHAPE_HXX
