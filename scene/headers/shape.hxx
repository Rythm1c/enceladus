#ifndef SHAPE_HXX
#define SHAPE_HXX

#include <vector>

#include "../../renderer/headers/buffer.hxx"
#include "../../renderer/headers/vertex.hxx"
#include "../../renderer/headers/drawable.hxx"

#include "../../math/headers/vec2.hxx"
#include "../../math/headers/vec3.hxx"
#include "../../math/headers/mat4.hxx"
#include "../../math/headers/transform.hxx"

class Core;

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
    //void draw(VkCommandBuffer cmd, VkPipelineLayout layout) const;

    const Drawable getDrawData() const;

    // ---- Transform ----------------------------------------------------------
    void setPosition(Vector3f translation);
    void setRotation(Quat rotation);
    void setScale(Vector3f size);

    void setTransform(const Transform &t);

    // converted to column major automatically so don't call it again
    Mat4x4 getModel() const { return m_model.toMat4x4().transpose(); }

protected:
    Core                 &m_core;
    Buffer                m_vertexBuffer;
    Buffer                m_indexBuffer;
    Transform             m_model;
    std::vector<Vertex3D> m_vertices;  // filled by buildGeometry()
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
        Core      &core,
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

class Cube : public Shape
{
public:
    explicit Cube(
        Core &core, float size = 0.5f,
        Vector3f color = {-1.0f, -1.0f, -1.0f});
private:
    float     m_size;
    Vector3f  m_color;
    void buildGeometry() override;
};

// =============================================================================
// Icosphere -- subdivided icosahedron, good for physics collision normals.
//
// subdivision = 0 : 12 verts,   20 faces  (original icosahedron)
// subdivision = 1 : 42 verts,   80 faces
// subdivision = 2 : 162 verts,  320 faces
// subdivision = 3 : 642 verts,  1280 faces  (recommended default)
// subdivision = 4 : 2562 verts, 5120 faces  (high quality, heavier)
// =============================================================================
class Icosphere : public Shape
{
public:
    explicit Icosphere(
        Core    &core,
        float    radius       = 0.5f,
        int      subdivisions = 3,
        Vector3f color        = {0.7f, 0.7f, 0.9f});
private:
    float    m_radius;
    int      m_subdivisions;
    Vector3f m_color;
    void buildGeometry() override;
};

// =============================================================================
// Plane -- flat XZ quad, normal pointing up (+Y).
// Use as physics floor / ground plane.
// =============================================================================
class Plane : public Shape
{
public:
    /**
     * @param core   Core reference.
     * @param size   Half-extent in X and Z (total side = size * 2).
     * @param color  Surface colour.
     * @param tileUV UV tile count across the plane (useful for grid textures).
     */
    explicit Plane(
        Core &core,
        float    size   = 10.0f,
        Vector3f color  = {0.4f, 0.45f, 0.4f},
        float    tileUV = 1.0f);
private:
    float    m_size;
    Vector3f m_color;
    float    m_tileUV;
    void buildGeometry() override;
};


#endif // SHAPE_HXX
