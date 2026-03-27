#ifndef VERTEX_HXX
#define VERTEX_HXX

#include <vulkan/vulkan.h>
#include <vector>
#include "../external/math/vec2.hxx"
#include "../external/math/vec3.hxx"

struct Vertex
{
    Vector2f pos;
    Vector3f col;

    static VkVertexInputBindingDescription                getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

#endif