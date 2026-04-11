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

Drawable Shape::getDrawData()
{
    return Drawable{
        .vertexBuffer     = m_vertexBuffer.get(),
        .indexBuffer      = m_indexBuffer.get(),   // VK_NULL_HANDLE if none
        .vertexCount      = static_cast<uint32_t>(m_vertices.size()),
        .indexCount       = static_cast<uint32_t>(m_indices.size()),
        .model            = getModel(),
        .material         = m_material.toUBO(),
    };
}

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
    Core    &core,
    Vector2f position,
    float    size,
    Vector3f colorA,
    Vector3f colorB,
    Vector3f colorC)
    : Shape(core, position),
      m_size(size),
      m_colorA(colorA),
      m_colorB(colorB),
      m_colorC(colorC) {}

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
        {0.9f, 0.2f, 0.3f},  // +X  red
        {0.2f, 0.3f, 0.8f},  // -X  blue
        {0.3f, 0.8f, 0.1f},  // +Y  green
        {0.9f, 0.8f, 0.1f},  // -Y  yellow
        {0.7f, 0.0f, 0.9f},  // +Z  orange
        {0.2f, 0.8f, 0.9f},  // -Z  purple
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
            {
                static_cast<uint16_t>(base+0), static_cast<uint16_t>(base+1), static_cast<uint16_t>(base+2),
                static_cast<uint16_t>(base+0), static_cast<uint16_t>(base+2), static_cast<uint16_t>(base+3)});
    }
}

Cube::Cube(Core &core, float size, Vector3f color)
    : Shape(core), m_size(size), m_color(color)
{}

CubeSphere::CubeSphere(
        Core    &core,
        float    radius, 
        int      subdivisions, 
        Vector3f color) 
        : Shape(core), m_radius(radius), m_subdivisions(subdivisions)
{}

void CubeSphere::buildGeometry()
{

    // Per-face colours (used when m_color == {-1,-1,-1})
    std::array<Vector3f, 3> faceColors = {{
        {0.9f, 0.1f, 0.2f},  //  red
        {0.2f, 0.3f, 0.8f},  //  blue
        {0.9f, 0.8f, 0.1f},  //  yellow
    }};

    if (m_color.x > 0.0f)
    {
        for(auto &color: faceColors)
            color = m_color;
    }

    const float step = 2.0f / (float)m_subdivisions;
    //  ________
    // |__|__|__|  subdivide a square and project(normalize) points to a sphere
    // |__|__|__|  <- square with three divisions 
    // |__|__|__|
    // ___ params _____________________________________
    // start     : starting point. Has to be the top-left corner of each face because of the the way the uv is calculated
    // latitude  : a direction vector sweeping along the latitude of the surface 
    // longitude : a direction vector sweeping along the longitude of the surface
    // color     : self explanatory
    auto subDivideFace =  [&](Vector3f start, Vector3f latitude, Vector3f longitude, Color3f color) -> std::vector<Vertex3D>
    {
        std::vector<Vertex3D> vertices;

        for(int j = 0; j < (m_subdivisions + 1); ++j)
        { 
            for (int i = 0; i < (m_subdivisions + 1); ++i) 
            {                                              
                Vector3f position = start + ((float)i * latitude) + ((float)j * longitude);

                float u = (float)i / (float)(m_subdivisions + 1);
                float v = 1.0f -  ((float)j / float(m_subdivisions + 1));

                // Use cube-to-sphere projection: normalize, then adjust by sqrt(1 + x²/2 + y²/2)
                // This reduces area distortion at cube corners compared to simple normalization
                Vector3f normalized = position.unit();
                /* float factor = std::sqrt(1.0f + normalized.x * normalized.x * 0.5f + normalized.y * normalized.y * 0.5f);
                Vector3f adjusted = normalized * (1.0f / factor); */

                vertices.push_back(Vertex3D{
                    .pos    = normalized * m_radius,
                    .normal = normalized,
                    .uv     = {u, v},
                    .col    = color,
                });
            }                                        
            
        }

        return vertices;
    };

    
    std::vector<Vertex3D> faces[6] = {
        // +Z face
        subDivideFace({-1.0, 1.0, 1.0}, {step , 0.0, 0.0}, {0.0,-step, 0.0}, faceColors[0]),
        // -Z face
        subDivideFace({1.0, 1.0,-1.0},  {-step, 0.0, 0.0}, {0.0,-step, 0.0}, faceColors[0]),
        // +Y face
        subDivideFace({-1.0, 1.0,-1.0}, {step , 0.0, 0.0}, {0.0, 0.0, step}, faceColors[1]),
        // -Y face
        subDivideFace({1.0,-1.0,-1.0},  {-step, 0.0, 0.0}, {0.0, 0.0, step}, faceColors[1]),
        // +X face
        subDivideFace({1.0, 1.0, 1.0},  {0.0, 0.0,-step},  {0.0,-step, 0.0}, faceColors[2]),
        // -X face
        subDivideFace({-1.0, 1.0,-1.0}, {0.0, 0.0, step},  {0.0,-step, 0.0}, faceColors[2])
    
    };

    // index generation
    for (int i = 0; i < 6; i++)
    {
        size_t base = m_vertices.size();
        for (uint16_t row = 0; row < m_subdivisions; ++row)
        {
            for(uint16_t col = 0; col < m_subdivisions; ++col)
            {
                
                uint16_t idx1 = ((uint16_t)m_subdivisions + 1) * row        + col + 0;
                uint16_t idx2 = ((uint16_t)m_subdivisions + 1) * (row + 1)  + col + 0;
                uint16_t idx3 = ((uint16_t)m_subdivisions + 1) * row        + col + 1;

                uint16_t idx4 = ((uint16_t)m_subdivisions + 1) * row        + col + 1;
                uint16_t idx5 = ((uint16_t)m_subdivisions + 1) * (row + 1)  + col + 0;
                uint16_t idx6 = ((uint16_t)m_subdivisions + 1) * (row + 1)  + col + 1;

                m_indices.push_back((uint16_t)base + idx1);
                m_indices.push_back((uint16_t)base + idx2);
                m_indices.push_back((uint16_t)base + idx3);

                m_indices.push_back((uint16_t)base + idx4);
                m_indices.push_back((uint16_t)base + idx5);
                m_indices.push_back((uint16_t)base + idx6);
            }
            
        }

        auto face = faces[i];
        m_vertices.insert(m_vertices.end(), face.begin(), face.end());
    }
}

Icosphere::Icosphere(Core &core, float radius, int subdivisions, Vector3f color)
    : Shape(core), m_radius(radius), m_subdivisions(subdivisions), m_color(color) {}

void Icosphere::buildGeometry()
{
    // Start with a regular icosahedron.
    // The golden ratio phi appears naturally in the coordinates of an icosahedron.
    const float v_angle  = std::atan(0.5f);  // ~26.565 degrees -- icosahedron geometry
    const float h_step   = 2.0f * PI / 5.0f; // 72 degrees between vertices

    // sin/cos of the vertical angle -- same for all belt vertices
    const float sinV = std::sin(v_angle);   //  0.4472 (y coordinate of upper belt)
    const float cosV = std::cos(v_angle);   //  0.8944 (xz radius of belt vertices)

    std::vector<Vertex3D>  vertices;
    std::vector<uint32_t>  indices;

    // Vertex counts per ring (6 each, not 5, for UV seam)
    // Ring indices:
    //   Ring 0 (top    poles):  0..5
    //   Ring 1 (upper  belt):   6..11
    //   Ring 2 (lower  belt):  12..17
    //   Ring 3 (bottom poles): 18..23

    // ---- Ring 0: top pole duplicates (y=1, all same 3D position) ----------
    // Each triangle that touches the top pole needs its own vertex with the
    // correct U coordinate for that triangle's centroid, otherwise all top
    // triangles share U=0 and the texture collapses to a point.
    for (int i = 0; i < 6; ++i)
    {
        float u = float(i) / 5.0f;
        vertices.push_back(Vertex3D{
            .pos    = {0.0f, m_radius, 0.0f},  // exactly at north pole
            .normal = {0.0f, 1.0f, 0.0f},
            .uv     = {u + 0.1f, 1.0f},         // offset by half-triangle width so
                                                // the U sits at the triangle centre
            .col    = m_color,
        });
    }

    // ---- Ring 1: upper belt (y = sin(v_angle)) -----------------------------
    for (int i = 0; i < 6; ++i)
    {
        float theta = i * h_step;
        float x     = std::cos(theta) * cosV;
        float z     = std::sin(theta) * cosV;
        float y     = sinV;

        // Normalize to unit sphere then scale by radius
        Vector3f p   = Vector3f(x, y, z); // already unit length for icosahedron
        Vector3f pos = p * m_radius;

        vertices.push_back(Vertex3D{
            .pos    = pos,
            .normal = p,                         // unit direction = correct normal on sphere
            .uv     = {float(i) / 5.0f, 0.75f},
            .col    = m_color,
        });
    }

    // ---- Ring 2: lower belt (y = -sin(v_angle)), rotated 36 degrees --------
    // The lower belt is offset by half a step (36 degrees) relative to the upper
    // belt -- this is the defining geometry of an icosahedron.
    for (int i = 0; i < 6; ++i)
    {
        float theta = (float(i) + 0.5f) * h_step; // 36-degree offset
        float x     = std::cos(theta) * cosV;
        float z     = std::sin(theta) * cosV;
        float y     = -sinV;

        Vector3f p   = Vector3f(x, y, z);
        Vector3f pos = p * m_radius;

        vertices.push_back(Vertex3D{
            .pos    = pos,
            .normal = p,
            .uv     = {(float(i) + 0.5f) / 5.0f, 0.25f},
            .col    = m_color,
        });
    }

    // ---- Ring 3: bottom pole duplicates (y=-1, all same 3D position) -------
    for (int i = 0; i < 6; ++i)
    {
        float u = (float(i) + 0.5f) / 5.0f;
        vertices.push_back(Vertex3D{
            .pos    = {0.0f, -m_radius, 0.0f},
            .normal = {0.0f, -1.0f, 0.0f},
            .uv     = {u, 0.0f},
            .col    = m_color,
        });
    }

    // =========================================================================
    // Indices -- 20 triangles total for a base icosahedron
    //
    //   5 top    triangles: ring0[i] -- ring1[i+1] -- ring1[i]
    //   10 middle triangles: 2 per column
    //   5 bottom triangles: ring2[i] -- ring2[i+1] -- ring3[i]
    // =========================================================================

    // Top cap: 5 triangles connecting pole to upper belt
    for (uint32_t i = 0; i < 5; ++i)
    {
        indices.push_back(i);           // ring 0 pole vertex i
        indices.push_back(6 + i + 1);  // ring 1 vertex i+1
        indices.push_back(6 + i);      // ring 1 vertex i
    }

    // Middle band: 10 triangles (2 per column)
    for (uint32_t i = 0; i < 5; ++i)
    {
        // Upper triangle of column (pointing down)
        indices.push_back(6  + i);      // ring1[i]
        indices.push_back(6  + i + 1);  // ring1[i+1]
        indices.push_back(12 + i);      // ring2[i]

        // Lower triangle of column (pointing up)
        indices.push_back(6  + i + 1);  // ring1[i+1]
        indices.push_back(12 + i + 1);  // ring2[i+1]
        indices.push_back(12 + i);      // ring2[i]
    }

    // Bottom cap: 5 triangles connecting lower belt to pole
    for (uint32_t i = 0; i < 5; ++i)
    {
        indices.push_back(12 + i);      // ring2[i]
        indices.push_back(12 + i + 1);  // ring2[i+1]
        indices.push_back(18 + i);      // ring3 pole vertex i
    }

    // =========================================================================
    // Subdivision
    //
    // Each triangle is split into 4 by inserting midpoints on each edge.
    // The midpoint is projected back onto the sphere (normalize then scale).
    //
    //        v1
    //       /  \
    //     m1----m3
    //     / \  / \
    //   v2---m2---v3
    //
    // UV midpoints are linearly interpolated -- this is an approximation but
    // works well for small triangles after a few subdivision levels.
    // The seam vertices (u=0 and u=1 at the same 3D position) cause some UV
    // discontinuity at the seam even after subdivision, which is a fundamental
    // limitation of cylindrical UV mapping on a closed surface.
    // =========================================================================

    auto midPoint = [&](const Vertex3D &a, const Vertex3D &b) -> Vertex3D
    {
        // Average positions then project onto sphere
        Vector3f mid    = ((a.pos + b.pos) * 0.5f).unit(); // unit direction
        Vector3f pos    = mid * m_radius;                   // scaled to radius

        // Linear UV interpolation
        Vector2f uv     = (a.uv + b.uv) * 0.5f;

        return Vertex3D{
            .pos    = pos,
            .normal = mid,  // unit direction is correct sphere normal
            .uv     = uv,
            .col    = m_color,
        };
    };

    for (int s = 0; s < m_subdivisions; ++s)
    {
        std::vector<Vertex3D>  newVerts;
        std::vector<uint32_t>  newIndices;
        newVerts.reserve  (vertices.size() * 4);
        newIndices.reserve(indices.size()  * 4);

        for (size_t j = 0; j < indices.size(); j += 3)
        {
            const Vertex3D &v1 = vertices[indices[j]];
            const Vertex3D &v2 = vertices[indices[j + 1]];
            const Vertex3D &v3 = vertices[indices[j + 2]];

            Vertex3D m1 = midPoint(v1, v2);
            Vertex3D m2 = midPoint(v2, v3);
            Vertex3D m3 = midPoint(v1, v3);

            // Each sub-triangle gets its own 3 vertices (no sharing).
            // Sharing would require a midpoint cache keyed on edge pairs --
            // for a physics engine this simpler approach is fine and avoids
            // the complexity of deduplication across UV seams.
            auto addTri = [&](const Vertex3D &a, const Vertex3D &b, const Vertex3D &c)
            {
                uint32_t base = static_cast<uint32_t>(newVerts.size());
                newVerts.push_back(a);
                newVerts.push_back(b);
                newVerts.push_back(c);
                newIndices.push_back(base);
                newIndices.push_back(base + 1);
                newIndices.push_back(base + 2);
            };

            addTri(v1, m1, m3);
            addTri(m1, v2, m2);
            addTri(m1, m2, m3);
            addTri(m3, m2, v3);
        }

        vertices = std::move(newVerts);
        indices  = std::move(newIndices);
    }

    m_vertices = std::move(vertices);
    m_indices.clear();
    m_indices.reserve(indices.size());
    for (auto idx : indices)
        m_indices.push_back(static_cast<uint32_t>(idx));

}


// =============================================================================
// Plane
// =============================================================================

Plane::Plane(Core &core, float size, Vector3f color, float tileUV)
    : Shape(core), m_size(size), m_color(color), m_tileUV(tileUV)
{
    m_material.useChecker = true;
}

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
