#ifndef MATERIAL_HXX
#define MATERIAL_HXX

#include "../../math/headers/vec3.hxx"
#include "ubo.hxx"

struct Material
{
    // ---- Checker (push constant) -------------------------------------------
    Vector3f colorA       = {1.0f, 1.0f, 1.0f};
    Vector3f colorB       = {0.1f, 0.1f, 0.1f};
    float    checkerScale = 4.0f;
    bool     useChecker   = false;

    // ---- PBR (UBO) ----------------------------------------------------------
    Vector3f albedo    = {1.0f, 1.0f, 1.0f};
    float    roughness = 0.5f;
    float    metallic  = 0.0f;
    float    ao        = 1.0f;

    // ---- Produce the UBO struct for upload ---------------------------------
    MaterialUBO toUBO() const
    {
        MaterialUBO ubo;
        ubo.albedo       = Vector4f(albedo.x, albedo.y, albedo.z, 1.0f);
        ubo.roughness    = roughness;
        ubo.metallic     = metallic;
        ubo.ao           = ao;
        ubo.useChecker   = useChecker ? 1.0f : 0.0f;
        ubo.colorA       = Vector4f(colorA.x, colorA.y, colorA.z, checkerScale);
        ubo.colorB       = Vector4f(colorB.x, colorB.y, colorB.z, 0.0f);
        return ubo;
    }

    // ========================================================================
    // Factory presets -- common real-world materials
    // ========================================================================

    // Plain solid colour with configurable roughness/metallic
    static Material solid(Vector3f color, float rough = 0.7f, float metal = 0.0f)
    {
        Material m;
        m.colorA     = color;
        m.albedo     = color;
        m.roughness  = rough;
        m.metallic   = metal;
        m.useChecker = false;
        return m;
    }

    // Rubber/plastic: non-metal, moderately rough
    static Material rubber(Vector3f color)
    {
        Material m;
        m.colorA     = color;
        m.albedo     = color;
        m.roughness  = 0.85f;
        m.metallic   = 0.0f;
        m.useChecker = false;
        return m;
    }

    // Metal: high reflectance, albedo tints the reflection
    static Material metal(Vector3f color, float rough = 0.2f)
    {
        Material m;
        m.colorA     = color;
        m.albedo     = color;
        m.roughness  = rough;
        m.metallic   = 1.0f;
        m.useChecker = false;
        return m;
    }

    // Checkerboard: useful for visualising rotation
    static Material checker(Vector3f a, Vector3f b,
                             float scale   = 8.0f,
                             float rough   = 0.8f,
                             float metal   = 0.0f)
    {
        Material m;
        m.colorA       = a;
        m.colorB       = b;
        m.checkerScale = scale;
        m.useChecker   = true;
        m.albedo       = (a + b) * 0.5f; // average for UBO (overridden per-fragment)
        m.roughness    = rough;
        m.metallic     = metal;
        return m;
    }

    // Stone/concrete: rough, non-metal
    static Material stone(Vector3f color = {0.5f, 0.5f, 0.5f})
    {
        return solid(color, 0.95f, 0.0f);
    }

    // Polished floor
    static Material polishedFloor(Vector3f color = {0.35f, 0.4f, 0.35f})
    {
        return solid(color, 0.3f, 0.0f);
    }
};

#endif