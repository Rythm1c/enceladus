#ifndef CORE_HXX
#define CORE_HXX

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

class Core
{
public:
    Core(SDL_Window *);
    ~Core() {};

private:
    VkInstance instance;

    VkPhysicalDevice physicalDevice;

    VkDevice device;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;

    void initVulkan(SDL_Window *);
    void createInstance(SDL_Window *);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface(SDL_Window *);
    void createSwapchain();
};

bool checkValidationLayerSupport();

#endif