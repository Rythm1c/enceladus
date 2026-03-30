#version 450

// ---- Inputs from vertex shader ---------------------------------------------
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

// ---- Output ----------------------------------------------------------------
layout(location = 0) out vec4 outColor;

void main()
{
    // Simple diffuse shading with a fixed directional light.
    // Replace this with a proper lighting UBO when you're ready.
    const vec3  lightDir  = normalize(vec3(1.0, -1.0, 1.0));
    const float ambient   = 0.15;
    const float diffuse   = max(dot(normalize(fragNormal), -lightDir), 0.0);

    vec3 lit = fragColor * (ambient + diffuse);
    outColor = vec4(lit, 1.0);
}
