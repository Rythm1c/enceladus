#include "../headers/shape.hxx"
#include "../headers/core.hxx"
#include "../external/math/quaternion.hxx"

#include <cassert>
#include <stdexcept>

// =============================================================================
// Shape
// =============================================================================

Shape::Shape(Core &core, Vector2f position)
    : m_core(core),
      m_vertexBuffer(core),
      m_indexBuffer(core) 
{
    model = Transform();
}

void Shape::upload()
{
    // Let the derived class fill m_vertices
    buildGeometry();

    assert(!m_vertices.empty() && "Shape::upload — buildGeometry() left m_vertices empty!");

    m_vertexBuffer.create(
        sizeof(Vertex3D) * m_vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_vertexBuffer.uploadDeviceLocal(m_vertices);

    if(!m_indices.empty())
    {
        m_indexBuffer.create(
            sizeof(uint16_t) * m_indices.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_indexBuffer.uploadDeviceLocal(m_indices);
    }
}

void Shape::draw(VkCommandBuffer cmd, VkPipelineLayout layout) const
{
    assert(m_vertexBuffer.isCreated() && "Shape::draw called before upload(). Call shape.upload() after construction.");

    ModelPushConstants pushConstants{
        .model = this->getModel(),
    };
    // Push the 2D offset
    vkCmdPushConstants(
        cmd,
        layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(ModelPushConstants),
        &pushConstants);

    // Bind the vertex buffer
    VkBuffer buffers[] = {m_vertexBuffer.get()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    if(!m_indices.empty())
    {
        vkCmdBindIndexBuffer(
            cmd, 
            m_indexBuffer.get(), 
            0, 
            VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(
            cmd, 
            static_cast<uint32_t>(m_indices.size()), 
            1, 
            0, 
            0, 
            0);
    }
    else
    {
    vkCmdDraw(cmd, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
    }
}

void Shape::setPosition(Vector3f translation)
{
    model.translation = translation;
}

void Shape::setRotation(float angleDeg, Vector3f axis)
{
    model.orientation = Quat(angleDeg, axis);
}

void Shape::setScale(Vector3f s)
{
    model.scaling = s;
}

// =============================================================================
// Triangle
// =============================================================================

Triangle::Triangle(
    Core &core,
    Vector2f position,
    float size,
    Vector3f colorA,
    Vector3f colorB,
    Vector3f colorC)
    : Shape(core, position),
      m_size(size),
      m_colorA(colorA),
      m_colorB(colorB),
      m_colorC(colorC)
{
}

void Triangle::buildGeometry()
{
    // Vertices are defined relative to the origin (0,0).
    // The per-frame push constant offset moves them to m_pushConstants.offset.
    //
    //        top
    //       /   \
    //  botL -----  botR
    //
    m_vertices = {
        Vertex3D{{0.0f,   -m_size, 1.0}, m_colorA},   // top centre
        Vertex3D{{-m_size, m_size, 1.0}, m_colorB},  // bottom left
        Vertex3D{{m_size,  m_size, 1.0}, m_colorC}, // bottom right
    };
}


void Cube::buildGeometry()
{
    const float s = m_size;

    // Per-face colours (used when m_color == {-1,-1,-1})
    const std::array<Vector3f, 6> faceColors = {{
        {1.0f, 0.2f, 0.2f},  // +X  red
        {0.2f, 0.6f, 1.0f},  // -X  blue
        {0.2f, 1.0f, 0.2f},  // +Y  green
        {1.0f, 1.0f, 0.2f},  // -Y  yellow
        {1.0f, 0.5f, 0.0f},  // +Z  orange
        {0.8f, 0.2f, 1.0f},  // -Z  purple
    }};

    const bool useUniform = (m_color.x >= 0.0f);

    // Each face: 4 unique vertices (shared edges would have different normals
    // on adjacent faces, so we can't share vertices across faces without
    // losing per-face normals for lighting).
    struct FaceDesc {
        Vector3f normal;
        // 4 corners in CCW winding order (Vulkan front face = clockwise by
        // default in our pipeline, but we set it in the rasteriser)
        std::array<Vector3f, 4> positions;
        std::array<Vector2f, 4> uvs;
    };

    const std::array<FaceDesc, 6> faces = {{
        // +X face (right)
        { { 1, 0, 0}, {{{ s,-s,-s},{ s, s,-s},{ s, s, s},{ s,-s, s}}},
          {{{0,1},{0,0},{1,0},{1,1}}} },
        // -X face (left)
        { {-1, 0, 0}, {{{-s,-s, s},{-s, s, s},{-s, s,-s},{-s,-s,-s}}},
          {{{0,1},{0,0},{1,0},{1,1}}} },
        // +Y face (up)
        { { 0, 1, 0}, {{{-s, s,-s},{-s, s, s},{ s, s, s},{ s, s,-s}}},
          {{{0,1},{0,0},{1,0},{1,1}}} },
        // -Y face (down)
        { { 0,-1, 0}, {{{-s,-s, s},{-s,-s,-s},{ s,-s,-s},{ s,-s, s}}},
          {{{0,1},{0,0},{1,0},{1,1}}} },
        // +Z face (front)
        { { 0, 0, 1}, {{{-s,-s, s},{-s, s, s},{ s, s, s},{ s,-s, s}}},
          {{{0,1},{0,0},{1,0},{1,1}}} },
        // -Z face (back)
        { { 0, 0,-1}, {{{ s,-s,-s},{ s, s,-s},{-s, s,-s},{-s,-s,-s}}},
          {{{0,1},{0,0},{1,0},{1,1}}} },
    }};

    m_vertices.clear();
    m_indices.clear();

    for (uint32_t f = 0; f < 6; ++f)
    {
        const auto &face  = faces[f];
        const Vector3f c = useUniform ? m_color : faceColors[f];
        const uint32_t  base = static_cast<uint32_t>(m_vertices.size());

        for (uint32_t v = 0; v < 4; ++v)
            m_vertices.push_back({face.positions[v], face.normal, face.uvs[v], c});

        // Two triangles per face (CCW winding): 0-1-2 and 0-2-3
        m_indices.insert(m_indices.end(),
            { static_cast<uint16_t>(base+0), static_cast<uint16_t>(base+1), static_cast<uint16_t>(base+2),
                static_cast<uint16_t>(base+0), static_cast<uint16_t>(base+2), static_cast<uint16_t>(base+3)});
    }
}

Cube::Cube(Core &core, float size, Vector3f color)
    : Shape(core), m_size(size), m_color(color)
{}
