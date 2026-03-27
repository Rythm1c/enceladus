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
    Core() = default;
    Core(SDL_Window *);

    Core(const Core &)            = delete;
    Core &operator=(const Core &) = delete;
    Core(Core &&)                 = delete;
    Core &operator=(Core &&)      = delete;

    ~Core();

    VkDevice         getDevice()              const { return m_device; }
    VkPhysicalDevice getPhysicaldevice()      const { return m_physicalDevice; }
    VkSurfaceKHR     getSurface()             const { return m_surface; }
    uint32_t         getGraphicsFamilyIndex() const { return m_queueFamilyIndices.graphicsFamily.value(); }
    uint32_t         getPresentFamilyIndex()  const { return m_queueFamilyIndices.presentFamily.value(); }
    VkQueue          getGraphicsQueue()       const { return m_graphicsQueue; }
    VkQueue          getPresentQueue()        const { return m_presentQueue; }

private:
    VkInstance m_instance                     = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice         = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue                   = VK_NULL_HANDLE;
    VkQueue m_presentQueue                    = VK_NULL_HANDLE;
    VkDevice m_device                         = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface                    = VK_NULL_HANDLE;

    QueueFamilyIndices m_queueFamilyIndices {};

    void initVulkan(SDL_Window *);
    void setupDebugMessenger();
    void createInstance(SDL_Window *);
    void createSurface(SDL_Window *);
    void pickPhysicalDevice();
    void createLogicalDevice();
    ;
};

#endif