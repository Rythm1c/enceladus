#include "../headers/core.hxx"
#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include <vector>
#include <set>

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

bool checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

Core::Core(SDL_Window *window)
    : instance(VK_NULL_HANDLE),
      physicalDevice(VK_NULL_HANDLE),
      graphicsFamilyIndex(std::nullopt),
      presentFamilyIndex(std::nullopt),
      graphicsQueue(VK_NULL_HANDLE),
      presentQueue(VK_NULL_HANDLE),
      device(VK_NULL_HANDLE),
      surface(VK_NULL_HANDLE),
      swapchain(VK_NULL_HANDLE)
{
    this->initVulkan(window);
}

void Core::initVulkan(SDL_Window *window)
{
    this->createInstance(window);
    this->createSurface(window);
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapchain();
}

void Core::createInstance(SDL_Window *window)
{
    /* if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    } */

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

    unsigned int deviceCount = 0;
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

    std::optional<unsigned int> _graphicsFamilyIndex;
    std::optional<unsigned int> _presentFamilyIndex;

    for (const auto &device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        unsigned int queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                _presentFamilyIndex = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                _graphicsFamilyIndex = i;
                break;
            }
            i++;
        }

        if (deviceFeatures.geometryShader &&
            _graphicsFamilyIndex.has_value() &&
            _presentFamilyIndex.has_value())
        {
            this->physicalDevice = device;
            this->graphicsFamilyIndex = _graphicsFamilyIndex;
            this->presentFamilyIndex = _presentFamilyIndex;
            break;
        }
    }

    if (!graphicsFamilyIndex.has_value())
    {
        throw std::runtime_error("failed to find a graphics queue family!");
    }

    if (!presentFamilyIndex.has_value())
    {
        throw std::runtime_error("failed to find a present queue family!");
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
        this->graphicsFamilyIndex.value(),
        this->presentFamilyIndex.value()};

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

    if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }
    else
    {
        std::cout << "Logical device created successfully" << std::endl;
    }

    vkGetDeviceQueue(this->device, graphicsFamilyIndex.value(), 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, presentFamilyIndex.value(), 0, &this->presentQueue);
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