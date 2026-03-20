#include "../headers/renderer.hxx"
#include <stdexcept>
#include <iostream>

Renderer::Renderer(RendererConfig config)
    : commandPool(VK_NULL_HANDLE)
{
    this->createFramebuffers(
        config.device,
        config.renderPass,
        config.swapChainImageViews,
        config.swapChainExtent);
    this->createCommandPool(
        config.device,
        config.graphicsQueueFamilyIndex);
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