#include "../headers/texture.hxx"
#include "../headers/core.hxx"
#include "../headers/buffer.hxx"

#include <stdexcept>

Texture::Texture(Core &core, const TextureConfig &config)
    : m_core(core), m_format(config.format)
{
    createImage(config);
    createImageView(config.aspectMask);
}

Texture::~Texture()
{
    VkDevice device = m_core.getDevice();
    vkDestroyImageView(device, m_view,   nullptr);
    vkDestroyImage    (device, m_image,  nullptr);
    vkFreeMemory      (device, m_memory, nullptr);
}

void Texture::createImage(const TextureConfig &config)
{
    VkDevice device = m_core.getDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = config.extent.width;
    imageInfo.extent.height = config.extent.height;
    imageInfo.extent.depth  = 1;          // 2D image — depth dimension is always 1
    imageInfo.mipLevels     = 1;          // no mipmaps for now
    imageInfo.arrayLayers   = 1;          // not a cubemap or array texture
    imageInfo.format        = config.format;
    imageInfo.tiling        = config.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // contents don't matter at creation
    imageInfo.usage         = config.usage;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;      // no MSAA yet
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;  // one queue family at a time

    if (vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
        throw std::runtime_error("Texture: vkCreateImage failed!");

    // Query what memory type the driver wants for this image.
    // Images have alignment and type requirements just like buffers.
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, m_image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memReqs.memoryTypeBits,
        m_core.getPhysicaldevice(),
        config.memoryProperties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
    {
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        throw std::runtime_error("Texture: vkAllocateMemory failed!");
    }

    vkBindImageMemory(device, m_image, m_memory, 0);
}

void Texture::createImageView(VkImageAspectFlags aspectMask)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = m_image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = m_format;
    viewInfo.subresourceRange.aspectMask     = aspectMask;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_core.getDevice(), &viewInfo, nullptr, &m_view) != VK_SUCCESS)
        throw std::runtime_error("Texture: vkCreateImageView failed!");
}