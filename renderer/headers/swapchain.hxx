#ifndef SWAPCHAIN_HXX
#define SWAPCHAIN_HXX

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <SDL2/SDL.h>

#include "texture.hxx"

class Core;

class Swapchain
{
private:
    Core                        &m_core;
    VkSwapchainKHR               m_handle      = VK_NULL_HANDLE;
    VkExtent2D                   m_extent      = {0, 0};
    VkFormat                     m_imageFormat = VK_FORMAT_UNDEFINED;
    /**
    * DepthBuffer — a Texture configured specifically for depth attachment use.
    *
    * Queries the physical device at construction to find the best supported
    * depth format (prefers D32_SFLOAT, falls back to D32_SFLOAT_S8_UINT or
    * D24_UNORM_S8_UINT).
    *
    * Owned by Swapchain — recreated alongside swapchain images on resize.
    *
    * Why a separate class instead of just calling Texture directly?
    *   - Encapsulates the format query logic (3 candidates, picked at runtime)
    *   - Makes the intent clear at the call site
    *   - Keeps Swapchain's constructor readable
    */
    std::unique_ptr<Texture>     m_depthBuffer = nullptr;
    std::vector<VkImage>         m_images;
    std::vector<VkImageView>     m_imageViews;
    

    void createSwapchain(SDL_Window *window);
    void createImageViews();
    void createDepthBuffer();

    VkFormat findDepthFormat();
public:
    explicit Swapchain(Core &core, SDL_Window* window);

    Swapchain(const Swapchain &)            = delete;
    Swapchain &operator=(const Swapchain &) = delete;
    Swapchain(Swapchain &&)                 = delete;
    Swapchain &operator=(Swapchain &&)      = delete;

    ~Swapchain();

    VkSwapchainKHR                  getHandle()      const { return m_handle; }
    VkFormat                        getFormat()      const { return m_imageFormat; }
    VkExtent2D                      getExtent()      const { return m_extent; }
    const std::vector<VkImageView> &getImageViews()  const { return m_imageViews; }
    VkImageView                     getDepthView()   const { return m_depthBuffer->getView(); }
    VkFormat                        getDepthFormat() const { return m_depthBuffer->getFormat(); }

};

#endif