#ifndef RENDERER_HXX
#define RENDERER_HXX

#include <vulkan/vulkan.h>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

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
    uint32_t currentFrame;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkFramebuffer> framebuffers;

    void createFramebuffers(
        VkDevice device,
        VkRenderPass renderPass,
        std::vector<VkImageView> swapChainImageViews,
        VkExtent2D swapChainExtent);

    void createCommandPool(
        VkDevice device,
        unsigned int queueFamilyIndex);

    void createCommandBuffers(VkDevice device);

    void createSyncObjects(VkDevice device);

    void recordFrame(RenderInfo info);

public:
    Renderer(RendererConfig config);

    ~Renderer() {}
    inline VkCommandBuffer getCommandBuffer() const { return this->commandBuffers[this->currentFrame]; }

    inline void clean(VkDevice device)
    {
        for (auto framebuffer : this->framebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkDestroyCommandPool(device, this->commandPool, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, this->imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, this->renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, this->inFlightFences[i], nullptr);
        }
    }

    void beginFrame(RenderInfo info);

    void presentFrame(RenderInfo info);
};

#endif