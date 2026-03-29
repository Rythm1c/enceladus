#include "../headers/buffer.hxx"
#include "../headers/core.hxx"

// =============================================================================
// Buffer — lifecycle
// =============================================================================

void Buffer::create(
    VkDeviceSize          size,
    VkBufferUsageFlags    usage,
    VkMemoryPropertyFlags properties)
{
    // Safe to call on an already-initialised buffer — free existing resources first.
    destroy();

    m_requestedSize    = size;
    m_memoryProperties = properties;

    // ----- Create the buffer object ------------------------------------------
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_core.getDevice(), &bufferInfo, nullptr, &m_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("Buffer::create — vkCreateBuffer failed!");
    }

    // ----- Query memory requirements -----------------------------------------
    vkGetBufferMemoryRequirements(m_core.getDevice(), m_handle, &m_memRequirements);
    m_allocatedSize = m_memRequirements.size; // may be > size due to alignment

    // ----- Allocate backing memory -------------------------------------------
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = m_memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        m_memRequirements.memoryTypeBits,
        m_core.getPhysicaldevice(),
        properties);

    if (vkAllocateMemory(m_core.getDevice(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
    {
        // Clean up the buffer handle we already created before throwing.
        vkDestroyBuffer(m_core.getDevice(), m_handle, nullptr);
        m_handle = VK_NULL_HANDLE;
        throw std::runtime_error("Buffer::create — vkAllocateMemory failed!");
    }

    vkBindBufferMemory(m_core.getDevice(), m_handle, m_memory, 0);
}

void Buffer::destroy()
{
    // Always unmap before freeing memory.
    if (m_mappedPtr != nullptr)
    {
        unmapPersistent();
    }

    if (m_handle != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_core.getDevice(), m_handle, nullptr);
        m_handle = VK_NULL_HANDLE;
    }

    if (m_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_core.getDevice(), m_memory, nullptr);
        m_memory = VK_NULL_HANDLE;
    }

    m_memRequirements  = {};
    m_requestedSize    = 0;
    m_allocatedSize    = 0;
    m_memoryProperties = 0;
}

// =============================================================================
// Buffer — persistent mapping
// =============================================================================

void Buffer::mapPersistent()
{
    assert(isCreated()    && "mapPersistent called on a buffer that has not been created!");
    assert(isHostVisible() && "mapPersistent requires a HOST_VISIBLE buffer!");
    assert(!isMapped()    && "Buffer is already persistently mapped!");

    vkMapMemory(m_core.getDevice(), m_memory, 0, m_allocatedSize, 0, &m_mappedPtr);
}

void Buffer::unmapPersistent()
{
    if (m_mappedPtr != nullptr)
    {
        vkUnmapMemory(m_core.getDevice(), m_memory);
        m_mappedPtr = nullptr;
    }
}

// =============================================================================
// Buffer — device-to-device copy
// =============================================================================

void Buffer::copyTo(Buffer &dst, VkCommandBuffer cmdBuf, VkDeviceSize copySize) const
{
    assert(isCreated()     && "copyTo: source buffer has not been created!");
    assert(dst.isCreated() && "copyTo: destination buffer has not been created!");

    // Default to the full source buffer if no explicit size is given.
    if (copySize == 0)
    {
        copySize = m_requestedSize;
    }

    assert(copySize <= dst.allocatedSize() &&
           "copyTo: destination buffer is too small for the copy!");

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size      = copySize;

    vkCmdCopyBuffer(cmdBuf, m_handle, dst.get(), 1, &copyRegion);
}

// =============================================================================
// Free function — memory type selection
// =============================================================================

uint32_t findMemoryType(
    uint32_t              typeFilter,
    VkPhysicalDevice      physicalDevice,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1u << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("findMemoryType — no suitable memory type found!");
}
