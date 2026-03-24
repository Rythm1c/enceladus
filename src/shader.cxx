#include "../headers/shader.hxx"
#include "../headers/utils.hxx"
#include <fstream>

Shader::Shader(VkDevice device, const char *filepath)
{
    auto code = readFile(filepath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(device, &createInfo, nullptr, &this->handle) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }
}
