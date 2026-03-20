#ifndef RENDERER_HXX
#define RENDERER_HXX

#include <vulkan/vulkan.h>
#include <vector>

struct RendererConfig
{
    VkDevice device;
    VkRenderPass renderPass;
    std::vector<VkImageView> swapChainImageViews;
    VkExtent2D swapChainExtent;
    unsigned int graphicsQueueFamilyIndex;
};

class Renderer
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    std::vector<VkFramebuffer> framebuffers;

    void createFramebuffers(
        VkDevice device,
        VkRenderPass renderPass,
        std::vector<VkImageView> swapChainImageViews,
        VkExtent2D swapChainExtent);

    void createCommandPool(
        VkDevice device,
        unsigned int queueFamilyIndex);
        
    void createCommandBuffer(VkDevice device);

public:
    Renderer(RendererConfig config);

    ~Renderer() {}
};

#endif