#include "../headers/buffer.hxx"
#include "../headers/core.hxx"

Buffer::Buffer(Buffer &&other) noexcept
    : ref_core(other.ref_core),
      m_handle(other.m_handle),
      m_bufferMemory(other.m_bufferMemory),
      m_size(other.m_size)
{
    other.m_handle = VK_NULL_HANDLE;
    other.m_bufferMemory = VK_NULL_HANDLE;
    other.m_size = 0;
}

void Buffer::destroy()
{
    if (m_handle != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(ref_core.getDevice(), m_handle, nullptr);
        m_handle = VK_NULL_HANDLE;
    }

    if (m_bufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(ref_core.getDevice(), m_bufferMemory, nullptr);
        m_bufferMemory = VK_NULL_HANDLE;
    }

    m_size = 0;
}

void Buffer::create(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    assert(this->m_handle == VK_NULL_HANDLE);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(this->ref_core.getDevice(), &bufferInfo, nullptr, &this->m_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    vkGetBufferMemoryRequirements(this->ref_core.getDevice(), this->m_handle, &this->m_memRequirements);
    this->m_size = this->m_memRequirements.size;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = m_memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(this->m_memRequirements.memoryTypeBits, ref_core.getPhysicaldevice(), properties);

    if (vkAllocateMemory(ref_core.getDevice(), &allocInfo, nullptr, &this->m_bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(ref_core.getDevice(), this->m_handle, this->m_bufferMemory, 0);
}

void copyBuffer(
    Core         &core,
    VkBuffer     src,
    VkBuffer     dst,
    VkDeviceSize size
){
    VkCommandBuffer cmd = core.beginSigleTimeCommands();

    VkBufferCopy region{};
    region.size = size;

    vkCmdCopyBuffer(
        cmd,
        src,
        dst,
        1,
        &region);
    
    core.endSingleTimeCommands(cmd);
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
