#version 450

// Minimal depth-only vertex shader for the shadow pass.
// Only position is needed -- colour, normal, and UV are ignored.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;   // unused -- must match Vertex3D binding
layout(location = 2) in vec2 uv;       // unused
layout(location = 3) in vec3 color;    // unused

// The light-space matrix is passed as a push constant so this shader
// needs no descriptor sets. This keeps the shadow pipeline layout minimal
// and avoids binding the full descriptor set during the shadow pass.
layout(push_constant) uniform ShadowPush {
    mat4 model;
    mat4 lightSpaceMatrix;
} push;

void main()
{
    gl_Position = push.lightSpaceMatrix * push.model * vec4(pos, 1.0);
}
