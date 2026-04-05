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
    vec4 cameraPos;         // xyz = world-space camera position (for specular)
    mat4 lightSpaceMatrix; // not used in frag -- already applied in vert
} light;

// sampler2DShadow -- the driver performs the depth comparison internally.
// texture() on this sampler returns a float in [0,1] representing the
// fraction of samples that passed the depth test (1.0 = fully lit, 0.0 = shadow).
layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;

// =============================================================================
// PCF shadow (3x3 kernel, 9 samples)
// =============================================================================
//   We sample the shadow map at 9 points in a 3x3 grid around the
//   projected position. Each sample uses the hardware depth comparison
//   (sampler2DShadow returns 0.0 or 1.0 per sample).
//   Averaging the 9 results gives a value between 0 and 1 that
//   represents how "in shadow" the fragment is -- producing soft edges.

layout(set = 1, binding = 0) uniform Material {
    vec4  albedo;      // xyz = base colour, w = unused
    float roughness;   // 0 = mirror, 1 = completely diffuse
    float metallic;    // 0 = dielectric (plastic/stone), 1 = metal
    float ao;          // ambient occlusion (1.0 = no occlusion)
    float useChecker;   // 0.0=solid, 1.0=checker
    vec4  colorA;       // xyz=checker colour A, w=checkerScale
    vec4  colorB;       // xyz=checker colour B, w=unused
} mat;

// ============================================================================
// Checkerboard pattern
// ============================================================================
vec3 checkerboard(vec2 uv)
{
    vec2  scaled = uv * mat.colorA.w;
    float parity = mod(floor(scaled.x) + floor(scaled.y), 2.0);
    return mix(mat.colorA.rgb, mat.colorB.rgb, parity);
}

float shadowPCF(vec4 lightSpacePos)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy   = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    float shadow    = 0.0;
    vec2  texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            shadow += texture(shadowMap,
                vec3(projCoords.xy + vec2(x,y) * texelSize, projCoords.z));

    return shadow / 9.0;
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    // Squaring roughness gives more perceptually linear control.
    // Artists expect roughness=0.5 to look "halfway" rough, which
    // the squared version approximates better than the raw value.
    float a     = roughness * roughness;
    float a2    = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (3.14159265 * denom * denom);
}

float geometrySchlickGGX(float NdotX, float roughness)
{
    float k = (roughness + 1.0);
    k       = (k * k) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float geometrySmith(float NdotL, float NdotV, float roughness)
{
    return geometrySchlickGGX(NdotL, roughness)
         * geometrySchlickGGX(NdotV, roughness);
}

void main()
{
    vec3 albedo = (mat.useChecker > 0.5) ? checkerboard(fragUV) * mat.albedo.rgb : mat.albedo.rgb;
    // Per-vertex colour as a tint multiplier (usually white = no effect)
    albedo *= fragColor;

    // ---- Setup vectors -----------------------------------------------------
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(light.cameraPos.xyz - fragWorldPos); // view direction
    vec3 L = normalize(-light.direction.xyz);               // toward light
    vec3 H = normalize(L + V);                              // half-vector

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // ---- F0: base reflectivity ---------------------------------------------
    // For dielectrics: 0.04 (most common dielectric IOR gives ~4% reflectance)
    // For metals: use albedo (metals have coloured reflections)
    vec3 F0 = mix(vec3(0.04), albedo, mat.metallic);

    // ---- Cook-Torrance specular BRDF ---------------------------------------
    float D = distributionGGX(N, H, mat.roughness);
    float G = geometrySmith(NdotL, NdotV, mat.roughness);
    vec3  F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Denominator: 4 * NdotL * NdotV (solid angle normalisation)
    // Clamped to prevent division by zero at grazing angles
    vec3 specular = (D * G * F) / max(4.0 * NdotL * NdotV, 0.001);

    // ---- Energy conservation -----------------------------------------------
    // ks = specular fraction = Fresnel term
    // kd = diffuse fraction  = (1 - ks) * (1 - metallic)
    //
    // The (1-metallic) term removes diffuse from metals entirely.
    // Metals don't have a diffuse component -- all light is reflected
    // specularly (just tinted by the albedo via F0).
    vec3 ks = F;
    vec3 kd = (vec3(1.0) - ks) * (1.0 - mat.metallic);

    // ---- Diffuse -----------------------------------------------------------
    // Lambertian diffuse: albedo / PI
    // Divided by PI because energy must be conserved over the hemisphere
    // (integral of cos(theta) over hemisphere = PI).
    vec3 diffuse = kd * albedo / 3.14159265;

    // ---- Incoming radiance -------------------------------------------------
    vec3 Li = light.color.xyz * light.color.w;

    // ---- Shadow ------------------------------------------------------------
    float shadow = shadowPCF(fragLightSpacePos);

    // ---- Direct lighting ---------------------------------------------------
    // Rendering equation: Lo = (diffuse + specular) * Li * NdotL * shadow
    // shadow modulates both diffuse and specular -- when in shadow, no
    // direct light contribution. A small floor (0.1) prevents pure black
    // since real shadows have secondary bounce light.
    vec3 Lo = (diffuse + specular) * Li * NdotL * max(shadow, 0.1);

    // ---- Ambient -----------------------------------------------------------
    // Approximation of indirect/environment light.
    // In full PBR this would be image-based lighting (an environment map).
    // For now: constant ambient * albedo * ao
    // ao (ambient occlusion) darkens crevices that receive less ambient light.
    vec3 ambient = light.ambient.xyz * albedo * mat.ao;

    // ---- Final colour ------------------------------------------------------
    vec3 color = ambient + Lo;

    // Reinhard tone mapping: compresses HDR values to [0,1] range.
    // Without this, bright specular highlights blow out to pure white.
    // color / (color + 1) maps 0->0, 0.5->0.33, 1->0.5, inf->1
    color = color / (color + vec3(1.0));

    // Gamma correction: convert from linear to sRGB (gamma 2.2 approximated
    // by the common 1/2.2 power). Monitors expect sRGB output.
    // Without this, colours look washed out and overly bright.
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
