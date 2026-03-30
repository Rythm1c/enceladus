#ifndef SWAPCHAIN_HXX
#define SWAPCHAIN_HXX

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <SDL2/SDL.h>

class Core;

class Swapchain
{
private:
    Core                        &m_core;
    VkSwapchainKHR               m_handle      = VK_NULL_HANDLE;
    VkExtent2D                   m_extent      = {0, 0};
    VkFormat                     m_imageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage>         m_images;
    std::vector<VkImageView>     m_imageViews;
    std::unique_ptr<DepthBuffer> m_depthBuffer;

    void createSwapchain(SDL_Window *window);
    void createImageViews();
    void createDepthBuffer();


public:
    explicit Swapchain(Core &core, SDL_Window* window);

    Swapchain(const Swapchain &)            = delete;
    Swapchain &operator=(const Swapchain &) = delete;
    Swapchain(Swapchain &&)                 = delete;
    Swapchain &operator=(Swapchain &&)      = delete;

    ~Swapchain();

    VkSwapchainKHR                  getHandle()     const { return m_handle; }
    VkFormat                        getFormat()     const { return m_imageFormat; }
    VkExtent2D                      getExtent()     const { return m_extent; }
    const std::vector<VkImageView> &getImageViews() const { return m_imageViews; }
    VkImageView                     getDepthView()   const { return m_depthBuffer->getView(); }
    VkFormat                        getDepthFormat() const { return m_depthBuffer->getFormat(); }

};

#endif