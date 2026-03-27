#ifndef BUFFER_HXX
#define BUFFER_HXX

#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

class Core;
class Buffer
{
public:
    explicit Buffer(Core &core) : ref_core(core) {}

    Buffer(const Buffer &)            = delete;
    Buffer &operator=(const Buffer &) = delete;

    Buffer(Buffer &&other)            noexcept;
    Buffer &operator=(Buffer &&other) noexcept
    {
        if (this != &other)
        {
            // Destroy current resources
            destroy();

            // Move ownership
            m_handle       = other.m_handle;
            m_bufferMemory = other.m_bufferMemory;
            m_size         = other.m_size;

            // Reset source
            other.m_handle       = VK_NULL_HANDLE;
            other.m_bufferMemory = VK_NULL_HANDLE;
            other.m_size         = 0;
        }

        return *this;
    }

    ~Buffer() { this->destroy(); }

    void create(
        VkDeviceSize          size,
        VkBufferUsageFlags    usage,
        VkMemoryPropertyFlags properties);

    template <typename T>
    void uploadData(const std::vector<T> &data);

    VkBuffer get()       const { return m_handle; }
    bool     isCreated() const { return  m_handle != VK_NULL_HANDLE; }

private:
    Core                &ref_core;
    VkBuffer             m_handle           = VK_NULL_HANDLE;
    VkDeviceMemory       m_bufferMemory     = VK_NULL_HANDLE;
    VkMemoryRequirements m_memRequirements{};
    VkDeviceSize         m_size             = 0;

    void destroy();
};

void copyBuffer(
    Core         &core,
    VkBuffer     src,
    VkBuffer     dst,
    VkDeviceSize size
);

uint32_t findMemoryType(
    uint32_t              typeFilter, 
    VkPhysicalDevice      physicalDevice, 
    VkMemoryPropertyFlags properties);

template<typename T>
Buffer createDeviceLocalBuffer(
    Core& core,
    const std::vector<T>& data,
    VkBufferUsageFlags usage);

#include "buffer.inl"

#endif