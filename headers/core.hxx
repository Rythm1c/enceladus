#ifndef CORE_HXX
#define CORE_HXX

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

#endif