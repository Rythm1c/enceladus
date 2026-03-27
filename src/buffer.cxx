#include "../headers/buffer.hxx"
#include "../headers/core.hxx"
#include <cassert>

Buffer::Buffer(Buffer &&other) noexcept
    : ref_core(other.ref_core),
      handle(other.handle),
      bufferMemory(other.bufferMemory),
      size(other.size)
{
    other.handle = VK_NULL_HANDLE;
    other.bufferMemory = VK_NULL_HANDLE;
    other.size = 0;
}

void Buffer::destroy()
{
    if (handle != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(ref_core.getDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }

    if (bufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(ref_core.getDevice(), bufferMemory, nullptr);
        bufferMemory = VK_NULL_HANDLE;
    }

    size = 0;
}

void Buffer::create(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    assert(this->handle == VK_NULL_HANDLE);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(this->ref_core.getDevice(), &bufferInfo, nullptr, &this->handle) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    vkGetBufferMemoryRequirements(this->ref_core.getDevice(), this->handle, &this->memRequirements);
    this->size = this->memRequirements.size;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(this->memRequirements.memoryTypeBits, ref_core.getPhysicaldevice(), properties);

    if (vkAllocateMemory(ref_core.getDevice(), &allocInfo, nullptr, &this->bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(ref_core.getDevice(), this->handle, this->bufferMemory, 0);
}

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
