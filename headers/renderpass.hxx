#ifndef RENDER_PASS_HXX
#define RENDER_PASS_HXX

#include <vulkan/vulkan.h>

class Core;

class RenderPass
{
    public:
    RenderPass(Core &core, VkFormat colorFormat, VkFormat depthFormat);
    ~RenderPass();

    RenderPass(const RenderPass &)            = delete;
    RenderPass &operator=(const RenderPass &) = delete;
    RenderPass(RenderPass &&)                 = delete;
    RenderPass &operator=(RenderPass &&)      = delete;

    inline VkRenderPass getHandle() const { return m_handle; }

private:
    Core &m_core;
    VkRenderPass m_handle;
};

#endif