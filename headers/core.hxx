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

#include "../headers/utils.hxx"

class Core
{
public:
    Core(SDL_Window *);
    ~Core() {};

    void clean();

    inline VkDevice getDevice() const { return device; }
    inline VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
    inline VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
    inline std::vector<VkImageView> getSwapChainImageViews() const { return swapChainImageViews; }
    inline VkSurfaceKHR getSurface() const { return surface; }
    inline unsigned int getGraphicsFamilyIndex() const { return queueFamilyIndices.graphicsFamily.value(); }
    inline unsigned int getPresentFamilyIndex() const { return queueFamilyIndices.presentFamily.value(); }
    inline VkSwapchainKHR getSwapChain() const { return swapchain; }
    inline VkQueue getGraphicsQueue() const { return graphicsQueue; }
    inline VkQueue getPresentQueue() const { return presentQueue; }

private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkDevice device;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    VkFormat swapChainImageFormat; // keep format from swapchain creation
    VkExtent2D swapChainExtent;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    void initVulkan(SDL_Window *);
    void setupDebugMessenger();
    void createInstance(SDL_Window *);
    void createSurface(SDL_Window *);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(SDL_Window *window);
    void createImageViews();
};

bool checkValidationLayerSupport();

#endif