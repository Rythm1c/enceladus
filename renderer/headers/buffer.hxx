#ifndef BUFFER_HXX
#define BUFFER_HXX

#include <iostream>
#include <vector>
#include <cassert>
#include <vulkan/vulkan.h>

class Core;

uint32_t findMemoryType(
    uint32_t              typeFilter, 
    VkPhysicalDevice      physicalDevice, 
    VkMemoryPropertyFlags properties);

class Buffer
{
public:
    explicit Buffer(Core &core) : m_core(core) {}

    // Non-copyable
    Buffer(const Buffer &)            = delete;
    Buffer &operator=(const Buffer &) = delete;

    // Movable
    Buffer(Buffer &&other) noexcept
        : m_core(other.m_core),
          m_handle(other.m_handle),
          m_memory(other.m_memory),
          m_memRequirements(other.m_memRequirements),
          m_requestedSize(other.m_requestedSize),
          m_allocatedSize(other.m_allocatedSize),
          m_memoryProperties(other.m_memoryProperties),
          m_mappedPtr(other.m_mappedPtr)
    {
        other.m_handle          = VK_NULL_HANDLE;
        other.m_memory          = VK_NULL_HANDLE;
        other.m_memRequirements = {};
        other.m_requestedSize   = 0;
        other.m_allocatedSize   = 0;
        other.m_memoryProperties = 0;
        other.m_mappedPtr       = nullptr;
    }

    Buffer &operator=(Buffer &&other) noexcept
    {
        if (this != &other)
        {
            destroy();

            m_handle           = other.m_handle;
            m_memory           = other.m_memory;
            m_memRequirements  = other.m_memRequirements;
            m_requestedSize    = other.m_requestedSize;
            m_allocatedSize    = other.m_allocatedSize;
            m_memoryProperties = other.m_memoryProperties;
            m_mappedPtr        = other.m_mappedPtr;

            other.m_handle           = VK_NULL_HANDLE;
            other.m_memory           = VK_NULL_HANDLE;
            other.m_memRequirements  = {};
            other.m_requestedSize    = 0;
            other.m_allocatedSize    = 0;
            other.m_memoryProperties = 0;
            other.m_mappedPtr        = nullptr;
        }
        return *this;
    }

    ~Buffer() { destroy(); }

    void create(
        VkDeviceSize          size,
        VkBufferUsageFlags    usage,
        VkMemoryPropertyFlags properties);

    template <typename T>
    void uploadData(const std::vector<T> &data, VkDeviceSize offset = 0);

    template <typename T>
    void uploadDeviceLocal(const std::vector<T> &array);

    /**
     * Persistent-map variant: keeps the buffer mapped across frames.
     * Only valid for HOST_VISIBLE buffers. Call once after create().
     */
    void mapPersistent();

    /** Unmap a persistently mapped buffer. Called automatically by destroy(). */
    void unmapPersistent();

    /**
     * Records a vkCmdCopyBuffer command to copy this buffer's contents
     * into @p dst. The caller is responsible for synchronisation.
     */
    void copyTo(Buffer &dst, VkCommandBuffer cmdBuf, VkDeviceSize size = 0) const;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    VkBuffer              get()               const { return m_handle; }
    VkDeviceSize          requestedSize()     const { return m_requestedSize; }
    VkDeviceSize          allocatedSize()     const { return m_allocatedSize; }
    VkMemoryPropertyFlags memoryProperties()  const { return m_memoryProperties; }
    bool                  isHostVisible()     const
    {
        return (m_memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    }
    bool isMapped()  const { return m_mappedPtr != nullptr; }
    bool isCreated() const { return m_handle != VK_NULL_HANDLE; }

private:
    Core             &m_core;
    VkBuffer               m_handle           = VK_NULL_HANDLE;
    VkDeviceMemory         m_memory           = VK_NULL_HANDLE;
    VkMemoryRequirements   m_memRequirements  = {};
    VkDeviceSize           m_requestedSize    = 0;
    VkDeviceSize           m_allocatedSize    = 0;   // may be > requestedSize due to alignment
    VkMemoryPropertyFlags  m_memoryProperties = 0;
    void                  *m_mappedPtr        = nullptr;

    void destroy();
};

#include "buffer.inl"

#endif