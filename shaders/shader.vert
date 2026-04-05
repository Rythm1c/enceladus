#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 color;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragWorldPos;
layout(location = 4) out vec4 fragLightSpacePos; // passed to frag for shadow lookup

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 0, binding = 1) uniform Light {
    vec4 direction;
    vec4 color;
    vec4 ambient;
    mat4 lightSpaceMatrix;
} light;

layout(push_constant) uniform PushConstants {
    mat4  model;
} push;


void main()
{
    vec4 worldPos   = push.model * vec4(pos, 1.0);
    gl_Position     = camera.proj * camera.view * worldPos;

    fragWorldPos       = worldPos.xyz;
    fragColor          = color;
    fragUV             = uv;
    fragNormal         = normalize(mat3(transpose(inverse(push.model))) * normal);
    fragLightSpacePos  = light.lightSpaceMatrix * worldPos;
}
