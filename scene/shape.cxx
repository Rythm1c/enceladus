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
        .material         = m_material.toUBO(),
        .model            = getModel()
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
            { static_cast<uint16_t>(base+0), static_cast<uint16_t>(base+1), static_cast<uint16_t>(base+2),
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
    std::array<Vector3f, 2> faceColors = {{
        {0.9f, 0.1f, 0.2f},  // +X  red
        //{0.2f, 0.3f, 0.8f},  // -X  blue
        //{0.3f, 0.8f, 0.1f},  // +Y  green
        {0.9f, 0.8f, 0.1f},  // -Y  yellow
        //{0.7f, 0.0f, 0.9f},  // +Z  orange
        //{0.2f, 0.8f, 0.9f},  // -Z  purple
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
                float factor = std::sqrt(1.0f + normalized.x * normalized.x * 0.5f + normalized.y * normalized.y * 0.5f);
                Vector3f adjusted = normalized * (1.0f / factor);

                vertices.push_back(Vertex3D{
                    .pos    = adjusted * m_radius,
                    .normal = adjusted,
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
        subDivideFace({1.0, 1.0, 1.0},  {0.0, 0.0,-step},  {0.0,-step, 0.0}, faceColors[1]),
        // -X face
        subDivideFace({-1.0, 1.0,-1.0}, {0.0, 0.0, step},  {0.0,-step, 0.0}, faceColors[1])
    
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
    : Shape(core), m_radius(radius), m_subdivisions(subdivisions), m_color(color)
{}
void Icosphere::buildGeometry()
{
    // Start with a regular icosahedron.
    // The golden ratio phi appears naturally in the coordinates of an icosahedron.
    const float v_angle = std::atan(0.5f);// ~26.565° angle from the Y axis to the second ring of vertices  
    const float h_angle = to_radians(72.0f);

    std::vector<Vertex3D> vertices;
    //top vertices(5 because of uv mappping)
    for(int i = 0; i < 5; ++i)
    {
        float theta = i * h_angle;
        float y = std::sin(3.0f * v_angle);
        vertices.push_back(Vertex3D{
            .pos    = {0.0f,    y, 0.0f},
            .normal = {0.0f, 1.0f, 0.0f},
            .uv     = {float(i) / 4.0f, 1.0f},
            .col    = m_color,
        });
    }
    // second ring vertices
    for (int i = 0; i < 5; ++i)
    {
        float theta = i * h_angle;
        float x = std::cos(theta) * std::cos(v_angle);
        float z = std::sin(theta) * std::cos(v_angle);
        float y = std::sin(v_angle);
        vertices.push_back(Vertex3D{
            .pos    = {x, y, z},
            .normal = {x, y, z},
            .uv     = {float(i) / 4.0f, 0.75f},
            .col    = m_color,
        });
    }
    //third ring vertices
    for (int i = 0; i < 5; ++i)
    {
        float theta = (i + 0.5f) * h_angle;
        float x = std::cos(theta) * std::cos(v_angle);
        float z = std::sin(theta) * std::cos(v_angle);
        float y = std::sin(-v_angle);
        vertices.push_back(Vertex3D{
            .pos    = {x, y, z},
            .normal = {x, y, z},
            .uv     = {(float(i) + 0.5f) / 4.0f, 0.25f},
            .col    = m_color,
        });
    }
    // bottom vertices(5 because of uv mappping)
    for(int i = 0; i < 5; ++i)
    {
        float theta = (i + 0.5f) * h_angle;
        float y = std::sin(-3.0f * v_angle);
        vertices.push_back(Vertex3D{
            .pos    = {0.0f,     y, 0.0f},
            .normal = {0.0f, -1.0f, 0.0f},
            .uv     = {(float(i) + 0.5f) / 4.0f, 0.0f},
            .col    = m_color,
        });
    }


    std::vector<uint16_t> indices;
    // top triangles
    for(uint16_t i = 0; i < 5; ++i)
    {
        indices.push_back(i);
        indices.push_back(5 + ((i + 1) % 5));
        indices.push_back(5 + i);
    }

    //middle triangles
    for(uint16_t i = 0; i < 5; ++i)
    {
        indices.push_back(5 + i);
        indices.push_back(5 + ((i + 1) % 5));
        indices.push_back(10 + i);
        
        indices.push_back(5 + ((i + 1) % 5));
        indices.push_back(10 + ((i + 1) % 5));
        indices.push_back(10 + i);
    }

    // bottom triangles
    for(uint16_t i = 0; i < 5; ++i)
    {
        indices.push_back(10 + i);
        indices.push_back(10 + ((i + 1) % 5));
        indices.push_back(15 + i);
    }

    // Subdivide triangles
    auto midPoint = [&](Vertex3D a, Vertex3D b) -> Vertex3D
    {
        Vector3f mid = ((a.pos + b.pos) * 0.5f).unit() * m_radius;
        Vector2f uv  = (a.uv  + b.uv)  * 0.5f;

        return Vertex3D{
            .pos    = mid,
            .normal = mid,
            .uv     = uv,
            .col    = m_color,
        };
    };


    for (int i = 0; i < m_subdivisions; ++i)
    {
        std::vector<Vertex3D> newVerts;
        std::vector<uint16_t> newIndices;

        for (size_t j = 0; j < indices.size(); j += 3)
        {
        // find 2 end vertices on the edges of the current row
        //          v1           
        //         /  \           
        //        /    \        
        //    m1 *------* m3  
        //      /  \  /  \       
        //    v2----*-----v3    
        //          m2
            auto addTriangle = [&](Vertex3D a, Vertex3D b, Vertex3D c)
            {
                uint16_t base = static_cast<uint16_t>(newVerts.size());
                newVerts.push_back(a);
                newVerts.push_back(b);
                newVerts.push_back(c);

                newIndices.push_back(base);
                newIndices.push_back(base + 1);
                newIndices.push_back(base + 2);
            };

            auto v1 = vertices[indices[j]];
            auto v2 = vertices[indices[j + 1]];
            auto v3 = vertices[indices[j + 2]];

            auto m1 = midPoint(v1, v2);
            auto m2 = midPoint(v2, v3);
            auto m3 = midPoint(v1, v3);

            addTriangle(v1, m1, m3);
            addTriangle(m1, v2, m2);
            addTriangle(m1, m2, m3);
            addTriangle(m3, m2, v3);
        }
        vertices = newVerts;
        indices = newIndices;
    }

    m_vertices = vertices;
    m_indices = indices;

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
