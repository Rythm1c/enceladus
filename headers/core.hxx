#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#ifndef CORE_HXX
#define CORE_HXX

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <optional>
#include <vector>

class Core
{
public:
    Core(SDL_Window *);
    ~Core() {};

private:
    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    std::optional<unsigned int> graphicsFamilyIndex;
    std::optional<unsigned int> presentFamilyIndex;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkDevice device;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    VkFormat swapChainImageFormat; // keep format from swapchain creation

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    void initVulkan(SDL_Window *);
    void createInstance(SDL_Window *);
    void createSurface(SDL_Window *);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(SDL_Window *window);
    void createImageViews();
};

bool checkValidationLayerSupport();

#endif