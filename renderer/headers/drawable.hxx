#ifndef DRAWABLE_HXX
#define DRAWABLE_HXX

#include <vulkan/vulkan.h>
#include "../../math/headers/mat4.hxx"

struct Drawable{
    VkBuffer     vertexBuffer;
    VkBuffer     indexBuffer;   // VK_NULL_HANDLE if none
    uint32_t     vertexCount;
    uint32_t     indexCount;
    Mat4x4       model;
};

#endif