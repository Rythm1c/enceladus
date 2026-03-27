#ifndef RENDERER_HXX
#define RENDERER_HXX

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

class Core;
class Pipeline;
class Shape;

const int MAX_FRAMES_IN_FLIGHT = 2;

struct RendererConfig
{
    Core                           &core;
    VkRenderPass                    renderPass;
    VkExtent2D                      swapChainExtent;
    const std::vector<VkImageView> &swapChainImageViews;
};


class Renderer
{
private:
    Core                        &m_core;
    VkRenderPass                 m_renderPass = VK_NULL_HANDLE;
    uint32_t                     m_currentFrame = 0;
    VkCommandPool                m_commandPool  = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore>     m_imageAvailableSemaphores;
    std::vector<VkSemaphore>     m_renderFinishedSemaphores;
    std::vector<VkFence>         m_inFlightFences;
    std::vector<VkFramebuffer>   m_framebuffers;

    void createFramebuffers(VkRenderPass renderPass, VkExtent2D extent,
                            const std::vector<VkImageView> &imageViews);
    void createCommandPool(uint32_t graphicsQueueFamilyIndex);
    void createCommandBuffers();
    void createSyncObjects();

public:
    explicit Renderer(RendererConfig &config);

    Renderer(const Renderer &)            = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&)                 = delete;
    Renderer &operator=(Renderer &&)      = delete;

    ~Renderer();

    VkCommandBuffer getCommandBuffer() const { return m_commandBuffers[m_currentFrame]; }

    // ----- Frame lifecycle ---------------------------------------------------
    // Returns the swapchain image index to use for this frame.
    uint32_t getFrame(VkSwapchainKHR swapchain, VkExtent2D extent);

    // Opens the command buffer and starts the render pass.
    void beginRecording(VkRenderPass renderPass, uint32_t imageIndex, VkExtent2D extent);

    // Binds a pipeline for subsequent draw calls.
    void bindPipeline(const Pipeline &pipeline);

    // Records draw commands for a shape (binds VBO, pushes constants, draws).
    void drawShape(const Shape &shape, const Pipeline &pipeline);

    // Ends the render pass and command buffer recording.
    void endRecording();

    // Submits and presents the current frame.
    void presentFrame(VkSwapchainKHR swapchain, uint32_t imageIndex);
};

#endif