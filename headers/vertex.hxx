#ifndef VERTEX_HXX
#define VERTEX_HXX

#include <vulkan/vulkan.h>
#include <vector>

struct Vertex
{
    float pos[2];
    float col[3];

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

struct Triangle
{
    Triangle(Vertex v1, Vertex v2, Vertex v3) : v1(v1), v2(v2), v3(v3) {}

private:
    Vertex v1;
    Vertex v2;
    Vertex v3;
};

#endif