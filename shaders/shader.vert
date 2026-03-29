#version 450

// ---- Vertex attributes (from the vertex buffer) ----------------------------
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 col;

// ---- Output to fragment shader ---------------------------------------------
layout(location = 0) out vec3 fragColor;

// ---- Descriptor set 0, binding 0 — Camera UBO ------------------------------
// This is the same struct as CameraUBO in C++.
// `set = 0` matches firstSet = 0 in vkCmdBindDescriptorSets.
// `binding = 0` matches the VkDescriptorSetLayoutBinding we created.
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

// ---- Push constant — model matrix ------------------------------------------
// Cheapest way to get per-object data to the shader.
// Matches ModelPushConstants in C++ exactly (64 bytes, offset 0).
layout(push_constant) uniform Model {
    mat4 model;
} push;

void main() {
    // MVP transform:
    //   model  → local space  to world space
    //   view   → world space  to camera/eye space
    //   proj   → camera space to clip space (Vulkan NDC)
    //
    // vec4(pos, 0.0, 1.0) promotes the 2D vertex to 3D with z=0 (flat on the
    // near plane) and w=1 (a position, not a direction).
    gl_Position = camera.proj * camera.view * push.model * vec4(pos, 0.0, 1.0);
    fragColor = col;
}
