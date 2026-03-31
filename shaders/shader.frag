#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec4 fragLightSpacePos; // position in light clip space

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform Light {
    vec4 direction;
    vec4 color;
    vec4 ambient;
    mat4 lightSpaceMatrix; // not used in frag -- already applied in vert
} light;

// sampler2DShadow -- the driver performs the depth comparison internally.
// texture() on this sampler returns a float in [0,1] representing the
// fraction of samples that passed the depth test (1.0 = fully lit, 0.0 = shadow).
layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;

// PCF: sample a 3x3 neighbourhood around the shadow map lookup point.
// Each sample is offset by one texel, then averaged.
// Result: smooth shadow edges instead of hard pixel-wide borders.
float shadowPCF(vec4 lightSpacePos)
{
    // Perspective divide: convert from clip space to NDC [-1,1]
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Remap XY from [-1,1] to [0,1] for texture lookup.
    // Z is already in [0,1] for Vulkan (unlike OpenGL which needs *0.5+0.5).
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Fragments outside the light frustum should not be shadowed.
    // The sampler's CLAMP_TO_BORDER + white border handles this for XY,
    // but we also guard Z explicitly.
    if (projCoords.z > 1.0) return 1.0;

    float shadow   = 0.0;
    vec2  texelSize = 1.0 / textureSize(shadowMap, 0);

    // 3x3 PCF kernel -- 9 samples
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offset    = vec2(x, y) * texelSize;
            // texture() with sampler2DShadow compares projCoords.z against
            // the stored depth at (projCoords.xy + offset).
            // Returns 1.0 if stored >= projCoords.z (lit), 0.0 if shadowed.
            shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z));
        }
    }

    return shadow / 9.0; // average the 9 samples
}

void main()
{
    vec3 norm     = normalize(fragNormal);
    vec3 lightDir = normalize(-light.direction.xyz);

    float diff   = max(dot(norm, lightDir), 0.0);
    float shadow = shadowPCF(fragLightSpacePos);

    // shadow = 1.0 means fully lit, 0.0 means fully in shadow.
    // We preserve a small fraction of diffuse even in shadow (0.1) so
    // shadowed areas don't go completely flat -- it approximates secondary bounce.
    vec3 ambient = light.ambient.xyz * fragColor;
    vec3 diffuse = light.color.xyz * light.color.w * diff * fragColor
                 * max(shadow, 0.1);

    outColor = vec4(ambient + diffuse, 1.0);
}
