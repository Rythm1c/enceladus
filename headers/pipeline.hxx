#ifndef PIPELINE_HXX
#define PIPELINE_HXX

#include <vulkan/vulkan.h>

class Pipeline
{

    VkPipelineLayout layout;
    VkPipeline handle;

public:
    Pipeline(VkDevice device, VkRenderPass renderPass, VkExtent2D swapChainExtent);
    ~Pipeline() {}

    inline VkPipeline getHandle() const { return this->handle; }

    inline void clean(VkDevice device)
    {
        vkDestroyPipeline(device, this->handle, nullptr);
        vkDestroyPipelineLayout(device, this->layout, nullptr);
    }
};

#endif