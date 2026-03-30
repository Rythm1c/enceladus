#ifndef TEXTURE_HXX
#define TEXTURE_HXX

#include <vulkan/vulkan.h>

class Core;

struct TextureConfig{
    VkExtent2D          extent     = {0, 0};
    VkFormat            format     = VK_FORMAT_UNDEFINED;

    // How the image will be used — drives memory placement and layout transitions.
    // Depth:   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    // Sampled: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    VkImageUsageFlags   usage      = 0;

    // Which aspects of the image the view exposes.
    // Depth:  VK_IMAGE_ASPECT_DEPTH_BIT
    // Colour: VK_IMAGE_ASPECT_COLOR_BIT
    VkImageAspectFlags  aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    // OPTIMAL  — GPU arranges pixels internally for fastest access (use this always
    //            unless you need CPU readback).
    // LINEAR   — row-major layout, allows CPU mapping (slower on GPU).
    VkImageTiling       tiling     = VK_IMAGE_TILING_OPTIMAL;

    // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT is correct for any GPU-only image
    // (depth buffers, render targets, sampled textures after upload).
    VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
};

class Texture
{
public:
    explicit Texture(Core &core,const TextureConfig &config);

    Texture(const Texture &)            = delete;
    Texture &operator=(const Texture &) = delete;
    Texture(Texture &&)                 = delete;
    Texture &operator=(Texture &&)      = delete;

    ~Texture();

    VkImage     getImage() const { return m_image; }
    VkImageView getView()  const { return m_view;  }
    VkFormat    getFormat() const { return m_format; }
private:

    Core           &m_core;
    VkImage         m_image  = VK_NULL_HANDLE;
    VkImageView     m_view   = VK_NULL_HANDLE;
    VkDeviceMemory  m_memory = VK_NULL_HANDLE;
    VkFormat        m_format = VK_FORMAT_UNDEFINED;

    void createImage(const TextureConfig &config);
    void createImageView(VkImageAspectFlags aspectMask);
};

#endif