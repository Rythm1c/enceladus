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

struct RenderInfo
{
    VkDevice device;
    uint32_t imageIndex;
    VkSwapchainKHR swapchain;
    VkExtent2D swapChainExtent;
    VkRenderPass renderPass;
    VkPipeline pipeline;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
};

class Renderer
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

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

    void createSyncObjects(VkDevice device);

    void recordFrame(RenderInfo info);

public:
    Renderer(RendererConfig config);

    ~Renderer() {}
    inline VkCommandBuffer getCommandBuffer() const { return this->commandBuffer; }

    inline void clean(VkDevice device)
    {
        for (auto framebuffer : this->framebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkDestroyCommandPool(device, this->commandPool, nullptr);
        vkDestroySemaphore(device, this->imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, this->renderFinishedSemaphore, nullptr);
        vkDestroyFence(device, this->inFlightFence, nullptr);
    }

    void beginFrame(RenderInfo info);

    void presentFrame(RenderInfo info);
};

#endif