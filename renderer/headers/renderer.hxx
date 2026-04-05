#ifndef RENDERER_HXX
#define RENDERER_HXX

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

class Core;
class Pipeline;
class Shape;
class GlobalDescriptor;
class MaterialDescriptor;

const int MAX_FRAMES_IN_FLIGHT = 2;

struct RendererConfig
{
    Core                           &core;
    VkRenderPass                    renderPass;
    VkExtent2D                      swapChainExtent;
    const std::vector<VkImageView> &swapChainImageViews;
    VkImageView                     depthImageView;
    GlobalDescriptor                &globalDescriptor;  // for per-frame UBO update + bind
    MaterialDescriptor              &materialDescriptor;  // for per-frame UBO update + bind

};

class Renderer
{
private:
    Core                        &m_core;
    GlobalDescriptor            &m_globalDescriptor;  // for per-frame UBO update + bind
    MaterialDescriptor          &m_materialDescriptor;  // for per-frame UBO update + bind
    VkRenderPass                 m_renderPass    = VK_NULL_HANDLE;
    uint32_t                     m_currentFrame  = 0;
    VkCommandPool                m_commandPool   = VK_NULL_HANDLE;
    float                        m_clearColor[3] = {0.1f, 0.1f, 0.1f};
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore>     m_imageAvailableSemaphores;
    std::vector<VkSemaphore>     m_renderFinishedSemaphores;
    std::vector<VkFence>         m_inFlightFences;
    std::vector<VkFramebuffer>   m_framebuffers;

    void createFramebuffers(
        VkRenderPass renderPass, VkExtent2D extent,
        const std::vector<VkImageView> &imageViews, VkImageView depthView);

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

    void clearColor(float r, float g, float b) { m_clearColor[0] = r; m_clearColor[1] = g; m_clearColor[2] = b; };

    // ----- Frame lifecycle ---------------------------------------------------
    // Returns the swapchain image index to use for this frame.
    uint32_t getFrame(VkSwapchainKHR swapchain, VkExtent2D extent);

    // Opens the command buffer and starts the render pass.
    void beginRecording();

    void beginRenderPass(VkRenderPass renderpass, uint32_t index, VkExtent2D extent);

    // Binds a pipeline for subsequent draw calls.
    void bindPipeline(const Pipeline &pipeline);

    /**
     * Bind the global descriptor set (set 0): camera + light + shadow map.
     * Call ONCE per frame after binding a pipeline, before any draw calls.
     * Updates the camera and light UBOs then records vkCmdBindDescriptorSets.
     */
    void bindGlobalDescriptors(const struct CameraUBO &camera, const struct LightUBO &light, VkPipelineLayout layout);

    /**
     * Update the material UBO and bind the material descriptor set (set 1).
     * Call once per shape, immediately before drawShape().
     * Only writes to the material buffer -- camera and light are untouched.
     */
    void bindMaterial(const struct MaterialUBO &material, VkPipelineLayout layout);

    // Records draw commands for a shape (binds VBO, pushes constants, draws).
    void draw(const struct Drawable &drawable, const Pipeline &pipeline);

    void endRenderPass();

    // Ends the render pass and command buffer recording.
    void endRecording();

    // Submits and presents the current frame.
    void presentFrame(VkSwapchainKHR swapchain, uint32_t imageIndex);

    
};

#endif