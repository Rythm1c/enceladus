#include "headers/shape.hxx"
#include "../renderer/headers/core.hxx"
#include "../math/headers/quaternion.hxx"

#include <cassert>
#include <array>
#include <unordered_map>
#include <cmath>


// =============================================================================
// Shape
// =============================================================================

Shape::Shape(Core &core, Vector2f position)
    : m_core(core),
      m_vertexBuffer(core),
      m_indexBuffer(core) 
{
    m_model = Transform();
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

const Drawable Shape::getDrawData() const
{
    return Drawable{
        .vertexBuffer     = m_vertexBuffer.get(),
        .indexBuffer      = m_indexBuffer.get(),   // VK_NULL_HANDLE if none
        .vertexCount      = static_cast<uint32_t>(m_vertices.size()),
        .indexCount       = static_cast<uint32_t>(m_indices.size()),
        .model            = getModel()
    };
}

/* void Shape::draw(VkCommandBuffer cmd, VkPipelineLayout layout) const
{
    assert(m_vertexBuffer.isCreated() && "Shape::draw called before upload(). Call shape.upload() after construction.");

    // Bind the vertex buffer
    VkBuffer buffers[] = {m_vertexBuffer.get()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    if(!m_indices.empty())
    {
        vkCmdBindIndexBuffer(cmd, m_indexBuffer.get(), 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }
    else
    {
    vkCmdDraw(cmd, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
    }
} */

void Shape::setPosition(Vector3f translation)
{
    m_model.translation = translation;
}

void Shape::setRotation(Quat rotation)
{
    m_model.orientation = rotation;
}

void Shape::setScale(Vector3f s)
{
    m_model.scaling = s;
}

void Shape::setTransform(const Transform &t)
{
    m_model = t;
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
        Vertex3D{{0.0f,    m_size, 1.0},{0.0, 0.0, 1.0},{0.5, 0.5}, m_colorA},   // top centre
        Vertex3D{{-m_size,-m_size, 1.0},{0.0, 0.0, 1.0},{0.0, 0.0}, m_colorB},  // bottom left
        Vertex3D{{m_size, -m_size, 1.0},{0.0, 0.0, 1.0},{0.0, 1.0}, m_colorC}, // bottom right
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
        // 4 corners in CCW winding order 
        std::array<Vector3f, 4> positions;
        std::array<Vector2f, 4> uvs;
    };

    const std::array<FaceDesc, 6> faces = {{
        // +X face (right)
        { { 1, 0, 0}, {{{ s,-s, s},{ s,-s,-s},{ s, s,-s},{ s, s, s}}},
                        {{{0,0},   {1,0},      {1,1},      {0,1}}} },
        // -X face (left)
        { {-1, 0, 0}, {{{-s,-s,-s},{-s,-s, s},{-s, s, s},{-s, s, -s}}},
                        {{{0,0},   {1,0},      {1,1},      {0,1}}} },
        // +Y face (up)
        { { 0, 1, 0}, {{{-s, s, s}, {s, s, s},{ s, s,-s},{ -s, s,-s}}},
                        {{{0,0},   {1,0},      {1,1},      {0,1}}} },
        // -Y face (down)
        { { 0,-1, 0}, {{{ s,-s, s}, {-s,-s, s}, {-s,-s,-s}, { s,-s,-s}}},
                        {{{0,1},     {0,0},      {1,0},      {1,1}}} },
        // +Z face (front)  towards camera by default since we look down -Z
        { { 0, 0, 1}, {{{-s,-s, s},{s, -s, s},{ s, s, s},{-s, s, s}}},
                        {{{0,0},   {1,0},      {1,1},      {0,1}}} },
        // -Z face (back)
        { { 0, 0,-1}, {{{ s,-s,-s},{-s,-s,-s},{-s, s,-s},{ s, s,-s}}},
                        {{{0,0},   {1,0},      {1,1},      {0,1}}} },
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

Icosphere::Icosphere(Core &core, float radius, int subdivisions, Vector3f color)
    : Shape(core), m_radius(radius), m_subdivisions(subdivisions), m_color(color)
{}
// TODO: fix this later 
// claude fucked it up 
void Icosphere::buildGeometry()
{
    // Start with a regular icosahedron.
    // The golden ratio phi appears naturally in the coordinates of an icosahedron.
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

    // 12 base vertices -- each pair of coordinates forms a golden rectangle.
    // All are normalized to the unit sphere.
    std::vector<Vector3f> positions = {
        Vector3f(-1,  phi, 0).unit(), Vector3f( 1,  phi, 0).unit(),
        Vector3f(-1, -phi, 0).unit(), Vector3f( 1, -phi, 0).unit(),
        Vector3f( 0, -1, phi).unit(), Vector3f( 0,  1, phi).unit(),
        Vector3f( 0, -1,-phi).unit(), Vector3f( 0,  1,-phi).unit(),
        Vector3f( phi, 0, -1).unit(), Vector3f( phi, 0,  1).unit(),
        Vector3f(-phi, 0, -1).unit(), Vector3f(-phi, 0,  1).unit(),
    };

    // 20 triangular faces of the base icosahedron
    std::vector<std::array<uint32_t, 3>> faces = {
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1},
    };

    // Subdivide each triangle into 4 smaller triangles N times.
    // Each edge midpoint is pushed back onto the unit sphere (normalize).
    // A cache avoids creating duplicate midpoint vertices for shared edges.
    for (int s = 0; s < m_subdivisions; ++s)
    {
        std::unordered_map<uint64_t, uint32_t> midpointCache;
        std::vector<std::array<uint32_t, 3>> newFaces;
        newFaces.reserve(faces.size() * 4);

        auto getMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t
        {
            // Pack two 32-bit indices into one 64-bit key (order-independent)
            uint64_t key = (uint64_t)std::min(a, b) << 32 | std::max(a, b);
            auto it = midpointCache.find(key);
            if (it != midpointCache.end()) return it->second;

            // New midpoint -- push onto unit sphere
            Vector3f mid = (Vector3f(positions[a] + positions[b]) * 0.5f).unit();
            uint32_t idx = static_cast<uint32_t>(positions.size());
            positions.push_back(mid);
            midpointCache[key] = idx;
            return idx;
        };

        for (auto &f : faces)
        {
            uint32_t a = getMidpoint(f[0], f[1]);
            uint32_t b = getMidpoint(f[1], f[2]);
            uint32_t c = getMidpoint(f[2], f[0]);
            newFaces.push_back({f[0], a, c});
            newFaces.push_back({f[1], b, a});
            newFaces.push_back({f[2], c, b});
            newFaces.push_back({a,    b, c});
        }

        faces = std::move(newFaces);
    }

    // Build final vertex and index arrays.
    // For a sphere the normal at each vertex == the vertex position (unit sphere).
    // UVs are spherical -- u from longitude, v from latitude.
    m_vertices.clear();
    m_indices.clear();
    m_vertices.reserve(positions.size());

    for (const auto &p : positions)
    {
        const Vector3f scaled = p * m_radius;
        const Vector3f normal = p; // already normalized
        const Vector2f uv = {
            0.5f + std::atan2(p.z, p.x) / (2.0f * PI),
            0.5f - std::asin(p.y) / PI
        };
        m_vertices.push_back({scaled, normal, uv, m_color});
    }

    m_indices.reserve(faces.size() * 3);
    for (const auto &f : faces)
        m_indices.insert(m_indices.end(), 
        {static_cast<uint16_t>(f[0]), 
            static_cast<uint16_t>(f[1]),
            static_cast<uint16_t>(f[2])});
}

// =============================================================================
// Plane
// =============================================================================

Plane::Plane(Core &core, float size, Vector3f color, float tileUV)
    : Shape(core), m_size(size), m_color(color), m_tileUV(tileUV)
{}

void Plane::buildGeometry()
{
    const float s = m_size;
    const float t = m_tileUV;
    const Vector3f normal = {0.0f, 1.0f, 0.0f}; // points up

    // 4 corners of a flat XZ quad, Y = 0
    m_vertices = {
        {{-s, 0.0f,  s}, normal, {0.0f, 0.0f}, m_color},
        {{-s, 0.0f, -s}, normal, {   t, 0.0f}, m_color},
        {{ s, 0.0f, -s}, normal, {   t,    t}, m_color},
        {{ s, 0.0f,  s}, normal, {0.0f,    t}, m_color},
    };
    m_indices = {0, 2, 1, 0, 3, 2};
}
