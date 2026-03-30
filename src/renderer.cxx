#include "../headers/renderer.hxx"
#include "../headers/core.hxx"
#include "../headers/pipeline.hxx"
#include "../headers/utils.hxx"
#include "../headers/shape.hxx"
#include "../headers/descriptor.hxx"
#include "../headers/ubo.hxx"
#include <stdexcept>
#include <array>
#include <iostream>

Renderer::Renderer(RendererConfig &config)
    : m_core(config.core),
      m_descriptor(config.descriptor),
      m_renderPass(config.renderPass)

{
    createFramebuffers(config.renderPass, config.swapChainExtent, config.swapChainImageViews, config.depthImageView);
    createCommandPool(m_core.getGraphicsFamilyIndex());
    createCommandBuffers();
    createSyncObjects();
}
Renderer::~Renderer()
{
    VkDevice device = m_core.getDevice();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, m_commandPool, nullptr);

    for (auto fb : m_framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);
}


uint32_t Renderer::getFrame(VkSwapchainKHR swapchain, VkExtent2D extent)
{
    VkDevice device = m_core.getDevice();

    vkWaitForFences(device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &m_inFlightFences[m_currentFrame]);

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame],
        VK_NULL_HANDLE, &imageIndex);

    return imageIndex;
}

void Renderer::beginRecording(VkRenderPass renderpass, uint32_t index, VkExtent2D extent)
{
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Renderer: vkBeginCommandBuffer failed!");

    // Two clear values -- order must match attachment order in RenderPass:
    //   [0] colour  → black, fully opaque
    //   [1] depth   → 1.0 (far plane) so every real fragment passes the LESS test
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderpass;
    renderPassInfo.framebuffer       = m_framebuffers[index];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float) extent.width;
    viewport.height   = (float) extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void Renderer::bindPipeline(const Pipeline &pipeline)
{
    vkCmdBindPipeline(m_commandBuffers[m_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getHandle());
}

void Renderer::bindDescriptors(const CameraUBO &camera, const LightUBO &light, VkPipelineLayout layout)
{
    // 1. Write fresh camera matrices into the UBO buffer for this frame.
    m_descriptor.updateCamera(m_currentFrame, camera);
    m_descriptor.updateLight (m_currentFrame, light);

    // 2. Bind the descriptor set so the shader can read the UBO.
    //
    // vkCmdBindDescriptorSets arguments:
    //   pipelineBindPoint → GRAPHICS (not compute)
    //   layout            → the pipeline layout that declares set 0
    //   firstSet = 0      → we are binding set 0 (matches `set = 0` in GLSL)
    //   descriptorSetCount = 1
    //   pDescriptorSets   → the set for this frame
    //   dynamicOffsetCount = 0  → no dynamic offsets
    VkDescriptorSet set = m_descriptor.getSet(m_currentFrame);
    vkCmdBindDescriptorSets(
        m_commandBuffers[m_currentFrame],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0, 1, &set,
        0, nullptr);
}

void Renderer::drawShape(const Shape &shape, const Pipeline &pipeline)
{
    shape.draw(m_commandBuffers[m_currentFrame], pipeline.getLayout());
}

void Renderer::endRecording()
{
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("Renderer: vkEndCommandBuffer failed!");
}

void Renderer::presentFrame(VkSwapchainKHR swapchain, uint32_t imageIndex)
{

    VkSemaphore          waitSemaphores[]   = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    VkCommandBuffer      cmd                = m_commandBuffers[m_currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(m_core.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Renderer: vkQueueSubmit failed!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    vkQueuePresentKHR(m_core.getPresentQueue(), &presentInfo);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
// =============================================================================
// Private setup
// =============================================================================

void Renderer::createFramebuffers(VkRenderPass renderPass, VkExtent2D extent, const std::vector<VkImageView> &imageViews, VkImageView depthView)
{
    m_framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        // Attachment order must match the render pass attachment descriptions:
        //   0 = colour (per-swapchain-image)
        //   1 = depth  (one shared image for all framebuffers -- only one
        //               frame is in flight per framebuffer at a time)
        std::array<VkImageView, 2> attachments = {imageViews[i], depthView};
        
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass      = renderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbInfo.pAttachments    = attachments.data();
        fbInfo.width           = extent.width;
        fbInfo.height          = extent.height;
        fbInfo.layers          = 1;

        if (vkCreateFramebuffer(m_core.getDevice(), &fbInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Renderer: vkCreateFramebuffer failed!");
    }

    std::cout << "Framebuffers created successfully" << std::endl;
}

void Renderer::createCommandPool(uint32_t graphicsQueueFamilyIndex)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    if (vkCreateCommandPool(m_core.getDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("Renderer: vkCreateCommandPool failed!");
    else
        std::cout << "Command pool created successfully" << std::endl;
}

void Renderer::createCommandBuffers()
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_core.getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Renderer: vkAllocateCommandBuffers failed!");
    else
        std::cout << "Command buffer allocated successfully" << std::endl;
}

void Renderer::createSyncObjects()
{

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = m_core.getDevice();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Renderer: failed to create sync objects!");
        }
    }

    std::cout << "Synchronization objects created successfully" << std::endl;
}
