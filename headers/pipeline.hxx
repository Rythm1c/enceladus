#ifndef PIPELINE_HXX
#define PIPELINE_HXX

#include <vulkan/vulkan.h>
#include "../headers/shader.hxx"

struct PipelineConfig
{
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkExtent2D swapChainExtent = {0, 0};
    Shader *vertShader = nullptr;
    Shader *fragShader = nullptr;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

class Pipeline
{

    VkPipelineLayout layout;
    VkPipeline handle;

public:
    Pipeline(PipelineConfig config);
    ~Pipeline() {}

    inline VkPipeline getHandle() const { return this->handle; }

    inline void clean(VkDevice device)
    {
        vkDestroyPipeline(device, this->handle, nullptr);
        vkDestroyPipelineLayout(device, this->layout, nullptr);
    }
};

#endif