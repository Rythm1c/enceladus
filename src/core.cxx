#include "../headers/core.hxx"
#include <../headers/constants.hxx>
#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include <set>
#include <cstdint>   // Necessary for uint32_t
#include <limits>    // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

Core::Core(SDL_Window *window)
    : instance(VK_NULL_HANDLE),
      debugMessenger(VK_NULL_HANDLE),
      physicalDevice(VK_NULL_HANDLE),
      queueFamilyIndices(QueueFamilyIndices()),
      graphicsQueue(VK_NULL_HANDLE),
      presentQueue(VK_NULL_HANDLE),
      device(VK_NULL_HANDLE),
      surface(VK_NULL_HANDLE)

{
    this->initVulkan(window);
}

void Core::initVulkan(SDL_Window *window)
{
    this->createInstance(window);
    this->setupDebugMessenger();
    this->createSurface(window);
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapchain(window);
    this->createImageViews();
}
void Core::clean()
{
    vkDestroyDevice(this->device, nullptr);
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
    }
    vkDestroyInstance(this->instance, nullptr);
}
void Core::createInstance(SDL_Window *window)
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }
    else
    {
        std::cout << "Validation layers are " << (enableValidationLayers ? "enabled" : "disabled") << std::endl;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Enceladus";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Enceladus Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    std::vector<const char *> extensions(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data()))
    {
        std::cerr << "Failed to get Vulkan instance extensions: " << SDL_GetError() << std::endl;
        extensions.clear();
        return;
    }
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<unsigned int>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &this->instance) != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
    }
    else
    {
        std::cout << "Vulkan instance created successfully" << std::endl;
    }
}

void Core::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Core::pickPhysicalDevice()
{

    unsigned int deviceCount = 0;
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        if (isDeviceSuitable(device, this->surface))
        {
            this->physicalDevice = device;
            this->queueFamilyIndices = findQueueFamilies(device, this->surface);
            break;
        }
    }

    if (this->physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    else
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(this->physicalDevice, &deviceProperties);
        std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
    }
}

void Core::createLogicalDevice()
{

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<unsigned int> uniqueQueueFamilies = {
        this->queueFamilyIndices.graphicsFamily.value(),
        this->queueFamilyIndices.presentFamily.value()};

    for (unsigned int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<unsigned int>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledExtensionCount = static_cast<unsigned int>(deviceExtensions.size());

    if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }
    else
    {
        std::cout << "Logical device created successfully" << std::endl;
    }

    vkGetDeviceQueue(this->device, this->queueFamilyIndices.graphicsFamily.value(), 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, this->queueFamilyIndices.presentFamily.value(), 0, &this->presentQueue);
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
