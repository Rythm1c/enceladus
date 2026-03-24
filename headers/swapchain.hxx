#ifndef SWAPCHAIN_HXX
#define SWAPCHAIN_HXX

#include <vulkan/vulkan.h>
#include <vector>
#include <SDL2/SDL.h>

struct SwapchainConfig
{
    VkDevice device;
    SDL_Window *window;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    unsigned int graphicsQueueFamilyIndex;
    unsigned int presentQueueFamilyIndex;
};

class Swapchain
{
    VkSwapchainKHR handle;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    void createSwapchain(SwapchainConfig &config);
    void createImageViews(VkDevice device);

public:
    Swapchain()
        : handle(VK_NULL_HANDLE),
          swapChainExtent({0, 0}),
          swapChainImageFormat(VK_FORMAT_UNDEFINED) {}

    Swapchain(SwapchainConfig &config);

    ~Swapchain() {}

    void clean(VkDevice device);

    inline VkSwapchainKHR getHandle() const { return handle; }
    inline VkFormat getFormat() const { return swapChainImageFormat; }
    inline VkExtent2D getExtent() const { return swapChainExtent; }
    inline std::vector<VkImageView> getImageViews() const { return swapChainImageViews; }
};

#endif