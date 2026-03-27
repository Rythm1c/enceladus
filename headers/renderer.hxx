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
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
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

public:
    Renderer(RendererConfig &config);

    ~Renderer() {}

    inline VkCommandBuffer getCommandBuffer() const { return this->commandBuffers[this->currentFrame]; }

    void clean(VkDevice device);

    uint32_t getFrame(VkDevice device, VkSwapchainKHR swapchain, VkExtent2D extent);

    void beginRecording(VkDevice device, VkRenderPass renderpass, uint32_t index, VkExtent2D extent);
    void endRecording();

    void bindPipeline(VkPipeline pipeline);

    void presentFrame(VkDevice device, VkSwapchainKHR swapchain, VkQueue graphicsQueue, VkQueue presentQueue, uint32_t index);
};

#endif