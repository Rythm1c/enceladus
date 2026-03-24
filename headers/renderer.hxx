#ifndef RENDERER_HXX
#define RENDERER_HXX

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

struct RendererConfig
{
    VkDevice device;
    VkRenderPass renderPass;
    unsigned int graphicsQueueFamilyIndex;
    VkSwapchainKHR swapchain;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
};

struct RenderInfo
{
    VkDevice device;
    uint32_t imageIndex;
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

    // sync objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkFramebuffer> framebuffers;

    void createFramebuffers(RendererConfig &config);

    void createCommandPool(RendererConfig &config);

    void createCommandBuffers(VkDevice device);

    void createSyncObjects(VkDevice device);

    void recordFrame(RenderInfo &info);

public:
    Renderer(RendererConfig &config);

    ~Renderer() {}

    inline VkCommandBuffer getCommandBuffer() const { return this->commandBuffers[this->currentFrame]; }

    void clean(VkDevice device);

    void beginFrame(RenderInfo &info);

    void presentFrame(RenderInfo &info);
};

#endif