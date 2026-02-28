#include "../headers/core.hxx"
#include <SDL2/SDL_vulkan.h>
#include <iostream>

Core::Core(SDL_Window *window)
    : instance(VK_NULL_HANDLE),
      physicalDevice(VK_NULL_HANDLE),
      device(VK_NULL_HANDLE),
      surface(VK_NULL_HANDLE),
      swapchain(VK_NULL_HANDLE)
{
    this->initVulkan(window);
}

void Core::initVulkan(SDL_Window *window)
{
    this->createInstance(window);
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSurface(window);
    this->createSwapchain();
}

void Core::createInstance(SDL_Window *window)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Phobos - Vulkan/SDL2";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Phobos Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    const char **extensions = new const char *[extensionCount];
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions))
    {
        std::cerr << "Failed to get Vulkan instance extensions: " << SDL_GetError() << std::endl;
        delete[] extensions;
        return;
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfo, nullptr, &this->instance) != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
    }
    else
    {
        std::cout << "Vulkan instance created successfully" << std::endl;
    }
}

void Core::pickPhysicalDevice()
{
}

void Core::createLogicalDevice()
{
}

void Core::createSurface(SDL_Window *window)
{
    if (!SDL_Vulkan_CreateSurface(window, this->instance, &this->surface))
    {
        std::cerr << "Failed to create Vulkan surface: " << SDL_GetError() << std::endl;
    }
    else
    {
        std::cout << "Vulkan surface created successfully" << std::endl;
    }
}

void Core::createSwapchain()
{
}