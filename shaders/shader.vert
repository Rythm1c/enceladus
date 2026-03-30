#version 450

// ---- Inputs (must match Vertex3D layout in vertex.hxx) ---------------------
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 color;

// ---- Outputs to fragment shader --------------------------------------------
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

// ---- Descriptor set 0, binding 0 -- Camera UBO ----------------------------
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

// ---- Push constant -- per-object model matrix ------------------------------
layout(push_constant) uniform Model {
    mat4 model;
} push;

void main()
{
    // Full MVP transform
    gl_Position = camera.proj * camera.view * push.model *  vec4(pos, 1.0);

    fragColor  = color;

    // Transform normal into world space for lighting calculations.
    // The normal matrix removes translation and handles non-uniform scale
    // correctly. transpose(inverse(...)) is expensive per-vertex; move this
    // to a UBO as a precomputed mat3 when you add lighting.
    fragNormal = mat3(transpose(inverse(push.model))) * normal;

    fragUV = uv;
}
