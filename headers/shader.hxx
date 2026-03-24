#ifndef SHADER_HXX
#define SHADER_HXX

#include <vulkan/vulkan.h>
#include <vector>

struct Shader
{
    Shader() : handle(VK_NULL_HANDLE) {}
    Shader(VkDevice device, const char *filepath);
    ~Shader() {}

    VkShaderModule getHandle() const { return this->handle; }

    inline void clean(VkDevice device)
    {
        vkDestroyShaderModule(device, this->handle, nullptr);
    }

private:
    VkShaderModule handle;

};

#endif