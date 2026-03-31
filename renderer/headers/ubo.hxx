#ifndef UBO_HXX
#define UBO_HXX

#include "../../math/headers/mat4.hxx"
#include "../../math/headers/vec4.hxx"

struct CameraUBO{
    Mat4x4 view;
    Mat4x4 proj;
};

/**
 * LightUBO -- uploaded once per frame, binding 1.
 *
 * vec4 is used throughout instead of vec3 because GLSL std140 layout
 * pads vec3 to 16 bytes anyway. Using vec4 explicitly prevents silent
 * alignment mismatches between C++ and GLSL.
 *
 *   direction.xyz  -- world-space direction the light is pointing
 *   color.xyz      -- light colour (linear RGB)
 *   color.w        -- intensity multiplier
 *   ambient.xyz    -- ambient colour (base light floor, prevents pure black shadows)
 *   ambient.w      -- unused padding
 */
struct LightUBO
{
    Vector4f direction        = { 0.5f, -1.0f, 0.5f, 0.0f }; // angled from top-right
    Vector4f color            = { 1.0f,  1.0f, 1.0f, 1.0f }; // white, full intensity
    Vector4f ambient          = { 0.15f, 0.15f, 0.2f, 0.0f }; // cool blue-tinted ambient
    Mat4x4   lightSpaceMatrix = Mat4x4::identity();

};

#endif