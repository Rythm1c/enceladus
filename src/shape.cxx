#include "../headers/shape.hxx"
#include "../headers/core.hxx"

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
    m_pushConstants.offset = position;
}

void Shape::upload()
{
    // Let the derived class fill m_vertices
    buildGeometry();

    assert(!m_vertices.empty() && "Shape::upload — buildGeometry() left m_vertices empty!");

    m_vertexBuffer = createDeviceLocalBuffer(
        m_core,
        m_vertices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    if(!m_indices.empty())
    {
        m_indexBuffer = createDeviceLocalBuffer(
        m_core,
        m_indices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }
}

void Shape::draw(VkCommandBuffer cmd, VkPipelineLayout layout) const
{
    assert(m_vertexBuffer.isCreated() &&
           "Shape::draw called before upload(). Call shape.upload() after construction.");

    // Push the 2D offset
    vkCmdPushConstants(
        cmd,
        layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(PushConstants2D),
        &m_pushConstants);

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
    // Draw
    vkCmdDraw(cmd, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
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
        Vertex{{0.0f, -m_size}, m_colorA},   // top centre
        Vertex{{m_size, m_size}, m_colorC},  // bottom right
        Vertex{{-m_size, m_size}, m_colorB}, // bottom left
    };
}
