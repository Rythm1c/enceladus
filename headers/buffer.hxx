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

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    Buffer(Buffer &&other) noexcept;
    Buffer &operator=(Buffer &&other) noexcept
    {
        if (this != &other)
        {
            // Destroy current resources
            destroy();

            // Move ownership
            handle = other.handle;
            bufferMemory = other.bufferMemory;
            size = other.size;

            // Reset source
            other.handle = VK_NULL_HANDLE;
            other.bufferMemory = VK_NULL_HANDLE;
            other.size = 0;
        }

        return *this;
    }

    ~Buffer() { this->destroy(); }

    void create(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    template <typename T>
    void uploadData(const std::vector<T> &data);

    VkBuffer get() const { return handle; }

    void destroy();

private:
    Core &ref_core;
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    VkMemoryRequirements memRequirements{};
    VkDeviceSize size = 0;
};

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);

#include "buffer.inl"

#endif