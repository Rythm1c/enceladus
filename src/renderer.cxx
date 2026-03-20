#include "../headers/renderer.hxx"
#include <stdexcept>
#include <iostream>

Renderer::Renderer(RendererConfig config)
    : currentFrame(0),
      commandPool(VK_NULL_HANDLE)
{
    this->createFramebuffers(
        config.device,
        config.renderPass,
        config.swapChainImageViews,
        config.swapChainExtent);
    this->createCommandPool(
        config.device,
        config.graphicsQueueFamilyIndex);
    this->createCommandBuffers(config.device);
    this->createSyncObjects(config.device);
}

void Renderer::createFramebuffers(
    VkDevice device,
    VkRenderPass renderPass,
    std::vector<VkImageView> swapChainImageViews,
    VkExtent2D swapChainExtent)
{
    this->framebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &this->framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    std::cout << "Framebuffers created successfully" << std::endl;
}

void Renderer::createCommandPool(VkDevice device, unsigned int queueFamilyIndex)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &this->commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
    else
    {
        std::cout << "Command pool created successfully" << std::endl;
    }
}

void Renderer::createCommandBuffers(VkDevice device)
{
    this->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (static_cast<uint32_t>(this->commandBuffers.size()));

    if (vkAllocateCommandBuffers(device, &allocInfo, this->commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffer!");
    }
    else
    {
        std::cout << "Command buffer allocated successfully" << std::endl;
    }
}

void Renderer::createSyncObjects(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateFence(device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create fence!");
        }
    }

    std::cout << "Synchronization objects created successfully" << std::endl;
}

void Renderer::recordFrame(RenderInfo info)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(this->commandBuffers[this->currentFrame], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = info.renderPass;
    renderPassInfo.framebuffer = this->framebuffers[info.imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = info.swapChainExtent;
    VkClearValue clearColor = {{{0.1f, 0.5f, 0.3f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(this->commandBuffers[this->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(this->commandBuffers[this->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(info.swapChainExtent.width);
    viewport.height = static_cast<float>(info.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(this->commandBuffers[this->currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = info.swapChainExtent;
    vkCmdSetScissor(this->commandBuffers[this->currentFrame], 0, 1, &scissor);
}

void Renderer::beginFrame(RenderInfo info)
{
    vkWaitForFences(info.device, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(info.device, 1, &this->inFlightFences[this->currentFrame]);

    vkAcquireNextImageKHR(info.device, info.swapchain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &info.imageIndex);

    vkResetCommandBuffer(this->commandBuffers[this->currentFrame], 0);

    this->recordFrame(info);
}

void Renderer::presentFrame(RenderInfo info)
{
    vkCmdEndRenderPass(this->commandBuffers[this->currentFrame]);
    if (vkEndCommandBuffer(this->commandBuffers[this->currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to end command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->commandBuffers[this->currentFrame];

    VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(info.graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {info.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &info.imageIndex;
    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(info.presentQueue, &presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}