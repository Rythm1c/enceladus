#include "headers/swapchain.hxx"
#include "headers/utils.hxx"
#include "headers/core.hxx"

#include <array>

Swapchain::Swapchain(Core &core, SDL_Window *window)
    : m_core(core)
{
    createSwapchain(window);
    createImageViews();
    createDepthBuffer();
}

Swapchain::~Swapchain()
{
    VkDevice device = m_core.getDevice();

    for (auto imageView : m_imageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, m_handle, nullptr);
}

void Swapchain::createSwapchain(SDL_Window *window)
{
    VkDevice device                 = m_core.getDevice();
    VkPhysicalDevice physicalDevice = m_core.getPhysicaldevice();
    VkSurfaceKHR surface            = m_core.getSurface();

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent                = chooseSwapExtent(window, swapChainSupport.capabilities);

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
    createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        m_core.getGraphicsFamilyIndex(),
        m_core.getPresentFamilyIndex()};

    if (m_core.getGraphicsFamilyIndex() != m_core.getPresentFamilyIndex())
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_handle) != VK_SUCCESS)
    {
        std::runtime_error("Failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(device, m_handle, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_handle, &imageCount, m_images.data());
    // remember the final format for later image‑view creation
    m_imageFormat = surfaceFormat.format;
    m_extent      = extent;
}

void Swapchain::createImageViews()
{
    m_imageViews.resize(m_images.size());

    for (size_t i = 0; i < m_images.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                           = m_images[i];
        createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                          = m_imageFormat;
        createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(m_core.getDevice(), &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image views!");
        }
    }

}
void Swapchain::createDepthBuffer()
{
    VkFormat depthFormat = findDepthFormat();

    TextureConfig config{};
    config.extent     = m_extent;
    config.format     = depthFormat;
    config.usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    config.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    config.tiling     = VK_IMAGE_TILING_OPTIMAL;
    config.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    m_depthBuffer = std::make_unique<Texture>(m_core, config);
}

VkFormat Swapchain::findDepthFormat()
{
    // Candidates in preference order:
    //   D32_SFLOAT          — 32-bit float depth, no stencil (best precision, widely supported)
    //   D32_SFLOAT_S8_UINT  — 32-bit float depth + 8-bit stencil
    //   D24_UNORM_S8_UINT   — 24-bit depth + 8-bit stencil (common on older hardware)
    constexpr std::array<VkFormat, 3> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    for (VkFormat format : candidates)
    {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(m_core.getPhysicaldevice(), format, &props);

        // OPTIMAL_TILING_FEATURES must include DEPTH_STENCIL_ATTACHMENT for
        // the format to be usable as a depth attachment with OPTIMAL tiling.
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return format;
    }

    throw std::runtime_error("DepthBuffer: no supported depth format found!");
}