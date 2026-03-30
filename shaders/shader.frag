#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform Light {
    vec4 direction; // xyz = direction toward light source (world space)
    vec4 color;     // xyz = colour, w = intensity
    vec4 ambient;   // xyz = ambient colour, w = unused
} light;

void main()
{
    vec3 norm     = normalize(fragNormal);
    vec3 lightDir = normalize(-light.direction.xyz); // negate: direction *to* light

    // Lambert diffuse -- dot product of surface normal and light direction.
    // max(0) clamps back-faces to zero rather than going negative.
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 ambient  = light.ambient.xyz * fragColor;
    vec3 diffuse  = light.color.xyz * light.color.w * diff * fragColor;

    outColor = vec4(ambient + diffuse, 1.0);
}
