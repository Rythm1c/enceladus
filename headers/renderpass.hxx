#ifndef RENDER_PASS_HXX
#define RENDER_PASS_HXX

#include "vulkan/vulkan.h"

struct RenderPass
{
    RenderPass() {}
    ~RenderPass() {};

private:
    vkRenderPass handle;
};

#endif