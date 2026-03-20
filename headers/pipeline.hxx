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
};

#endif