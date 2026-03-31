#include "headers/shader.hxx"
#include "headers/core.hxx"
#include "headers/utils.hxx"
#include <fstream>

Shader::Shader(Core &core, const char *filepath)
    : m_core(core)
{
    auto code = readFile(filepath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(m_core.getDevice(), &createInfo, nullptr, &m_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }
}

Shader::~Shader()
{
    if (m_handle != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_core.getDevice(), m_handle, nullptr);
    }
}
