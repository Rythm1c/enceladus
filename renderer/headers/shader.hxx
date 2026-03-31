#ifndef SHADER_HXX
#define SHADER_HXX

#include <vulkan/vulkan.h>
#include <vector>

class Core;
struct Shader
{
    explicit Shader(Core &core) : m_core(core) {}
    Shader(Core &core, const char *filepath);
    ~Shader();

    VkShaderModule getHandle() const { return m_handle; }

private:
    Core &m_core;
    VkShaderModule m_handle = VK_NULL_HANDLE;
};

#endif