#include "../headers/renderer.hxx"
#include <stdexcept>
#include <iostream>

Renderer::Renderer(RendererConfig config)
    : commandPool(VK_NULL_HANDLE),
      commandBuffer(VK_NULL_HANDLE),
      imageAvailableSemaphore(VK_NULL_HANDLE),
      renderFinishedSemaphore(VK_NULL_HANDLE),
      inFlightFence(VK_NULL_HANDLE)
{
    this->createFramebuffers(
        config.device,
        config.renderPass,
        config.swapChainImageViews,
        config.swapChainExtent);
    this->createCommandPool(
        config.device,
        config.graphicsQueueFamilyIndex);
    this->createCommandBuffer(config.device);
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

void Renderer::createCommandBuffer(VkDevice device)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &this->commandBuffer) != VK_SUCCESS)
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

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create semaphores!");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(device, &fenceInfo, nullptr, &this->inFlightFence) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create fence!");
    }
    else
    {
        std::cout << "Synchronization objects created successfully" << std::endl;
    }
}

void Renderer::recordFrame(RenderInfo info)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(this->commandBuffer, &beginInfo) != VK_SUCCESS)
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

    vkCmdBeginRenderPass(this->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(info.swapChainExtent.width);
    viewport.height = static_cast<float>(info.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(this->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = info.swapChainExtent;
    vkCmdSetScissor(this->commandBuffer, 0, 1, &scissor);
}

void Renderer::beginFrame(RenderInfo info)
{
    vkWaitForFences(info.device, 1, &this->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(info.device, 1, &this->inFlightFence);

    vkAcquireNextImageKHR(info.device, info.swapchain, UINT64_MAX, this->imageAvailableSemaphore, VK_NULL_HANDLE, &info.imageIndex);

    vkResetCommandBuffer(this->commandBuffer, 0);

    this->recordFrame(info);
}

void Renderer::presentFrame(RenderInfo info)
{
    vkCmdEndRenderPass(this->commandBuffer);
    if (vkEndCommandBuffer(this->commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to end command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->commandBuffer;

    VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(info.graphicsQueue, 1, &submitInfo, this->inFlightFence) != VK_SUCCESS)
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
}