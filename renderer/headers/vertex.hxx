#ifndef VERTEX_HXX
#define VERTEX_HXX

#include <vulkan/vulkan.h>
#include <vector>
#include "../../math/headers/vec2.hxx"
#include "../../math/headers/vec3.hxx"

/**
 * Vertex3D -- the single vertex type used by all shapes.
 *
 * Layout:
 *   location 0 -- pos    (vec3)
 *   location 1 -- normal (vec3)
 *   location 2 -- uv     (vec2)
 *   location 3 -- color  (vec3)
 */
struct Vertex3D
{
    Vector3f pos;
    Vector3f normal;
    Vector2f uv;
    Vector3f col;

    static VkVertexInputBindingDescription                getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

#endif