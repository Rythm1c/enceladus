#include "../headers/vertex.hxx"

VkVertexInputBindingDescription Vertex3D::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex3D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex3D::getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.push_back(
        {
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex3D, pos),

        });

    attributeDescriptions.push_back(
        {
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex3D, normal),
        });

    attributeDescriptions.push_back(
        {
            .location = 2,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = offsetof(Vertex3D, uv),
        });

    attributeDescriptions.push_back(
        {
            .location = 3,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex3D, col),
        });

    return attributeDescriptions;
}
