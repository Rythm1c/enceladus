#ifndef PIPELINE_HXX
#define PIPELINE_HXX

#include <vulkan/vulkan.h>
#include <vector>
#include "../headers/shader.hxx"

class Core;
class Shader;

/// @brief set topology to VK_PRIMITIVE_TOPOLOGY_LINE_LIST 
/// when rendering in wireframe mode( wireframe = ture)
struct PipelineConfig
{
    Core                                          &core;
    VkRenderPass                                   renderPass            = VK_NULL_HANDLE;
    VkExtent2D                                     swapChainExtent       = {0, 0};
    Shader                                        *vertShader            = nullptr;
    Shader                                        *fragShader            = nullptr; // may be nullptr for depthOnly
    VkPrimitiveTopology                            topology              = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkDescriptorSetLayout                          descriptorSetLayout   = VK_NULL_HANDLE;
    bool                                           wireframe             = false;
    bool                                           depthOnly             = false; // mostly for shadow map pipelines
    std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    std::vector<VkPushConstantRange>               pushConstantRanges;


};

class Pipeline
{
    Core            &m_core;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkPipeline       m_handle = VK_NULL_HANDLE;

public:
    explicit Pipeline(PipelineConfig &config);
    Pipeline(const Pipeline &)            = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&)                 = delete;
    Pipeline &operator=(Pipeline &&)      = delete;

    ~Pipeline();

    VkPipeline       getHandle() const { return m_handle; }
    VkPipelineLayout getLayout() const { return m_layout; }
};

#endif