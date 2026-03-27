#ifndef SHAPE_HXX
#define SHAPE_HXX

#include <vector>
#include <vulkan/vulkan.h>

#include "buffer.hxx"
#include "vertex.hxx"

#include "../external/math/vec2.hxx"
#include "../external/math/vec3.hxx"

class Core;

// =============================================================================
// Push constants — 2D offset only for now; expand to mat4 once depth is ready
// =============================================================================
struct PushConstants2D
{
    Vector2f offset = {0.0f, 0.0f};
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
    void        setPosition(Vector2f pos) { m_pushConstants.offset = pos; }
    Vector2f    getPosition()        const { return m_pushConstants.offset; }

protected:
    Core               &m_core;
    Buffer              m_vertexBuffer;
    PushConstants2D     m_pushConstants;
    std::vector<Vertex> m_vertices;  // filled by buildGeometry()

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
