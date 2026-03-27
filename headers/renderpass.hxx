#ifndef RENDER_PASS_HXX
#define RENDER_PASS_HXX

#include "vulkan/vulkan.h"

struct RenderPass
{
    RenderPass(VkDevice device, VkFormat swapChainImageFormat);
    ~RenderPass() {}

    inline VkRenderPass getHandle() const { return this->handle; }

    inline void clean(VkDevice device)
    {
        vkDestroyRenderPass(device, this->handle, nullptr);
    }

private:
    VkRenderPass handle;
};

#endif