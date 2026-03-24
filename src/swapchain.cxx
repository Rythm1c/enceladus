#include "../headers/swapchain.hxx"

Swapchain::Swapchain(SwapchainConfig config)
    : handle(VK_NULL_HANDLE),
      swapChainExtent({0, 0}),
      swapChainImageFormat(VK_FORMAT_UNDEFINED)
{
    this->createSwapchain(device, surface, window);
    this->createImageViews();
}

void Swapchain::clean()
{
    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Swapchain::createSwapchain(SwapchainConfig config)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(config.physicalDevice, config.surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(config.window, swapChainSupport.capabilities);

    std::string formatName = getFormatName(surfaceFormat.format);
    std::string presentModeName = getPresentModeName(presentMode);
    std::cout << "Selected swapchain format: " << formatName << std::endl;
    std::cout << "Selected swapchain present mode: " << presentModeName << std::endl;
    std::cout << "Selected swapchain extent: " << extent.width << "x" << extent.height << std::endl;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = config.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {config.graphicsQueueFamilyIndex, config.presentQueueFamilyIndex};

    if (config.graphicsQueueFamilyIndex != config.presentQueueFamilyIndex)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(config.device, &createInfo, nullptr, &this->swapchain) != VK_SUCCESS)
    {
        std::runtime_error("Failed to create swapchain");
    }
    else
    {
        std::cout << "swapchain created successfully" << std::endl;
    }

    vkGetSwapchainImagesKHR(config.device, this->swapchain, &imageCount, nullptr);
    this->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(config.device, this->swapchain, &imageCount, this->swapChainImages.data());
    // remember the final format for later image‑view creation
    this->swapChainImageFormat = surfaceFormat.format;
    this->swapChainExtent = extent;
}

void Swapchain::createImageViews(VkDevice device)
{
    this->swapChainImageViews.resize(this->swapChainImages.size());

    for (size_t i = 0; i < this->swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = this->swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = this->swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &this->swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image views!");
        }
    }

    std::cout << "Swapchain image views created successfully" << std::endl;
}
