#include "core.hxx"
template <typename T>
void Buffer::uploadData(const std::vector<T> &array, VkDeviceSize offset)
{
    assert(isCreated() && "uploadData called on a buffer that has not been created!");
    assert(isHostVisible() &&
            "uploadData requires a HOST_VISIBLE buffer. "
            "For DEVICE_LOCAL buffers use a staging buffer + copyTo().");

    const VkDeviceSize dataSize = static_cast<VkDeviceSize>(sizeof(T) * array.size());
    assert(offset + dataSize <= m_allocatedSize &&
            "Buffer is too small for the upload data (offset + dataSize exceeds allocated size)!");

    if (m_mappedPtr)
    {
        // Persistent mapping — write directly
        memcpy(static_cast<char *>(m_mappedPtr) + offset, array.data(), static_cast<size_t>(dataSize));
    }
    else
    {
        // Transient mapping
        void *mapped = nullptr;
        vkMapMemory(m_core.getDevice(), m_memory, offset, dataSize, 0, &mapped);
        memcpy(mapped, array.data(), static_cast<size_t>(dataSize));
        vkUnmapMemory(m_core.getDevice(), m_memory);
    }
}

template <typename T>
void Buffer::uploadDeviceLocal(const std::vector<T> &array)
{
    assert(isCreated() && "uploadDeviceLocal called on a buffer that has not been created!");
    assert(!isHostVisible() &&
            "uploadDeviceLocal is for DEVICE_LOCAL buffers. "
            "Use uploadData() for HOST_VISIBLE buffers.");

    const VkDeviceSize dataSize = static_cast<VkDeviceSize>(sizeof(T) * array.size());
    assert(dataSize <= m_allocatedSize &&
            "Buffer is too small for the upload data!");

    // ----- Step 1 & 2: staging buffer + memcpy -------------------------------
    // The staging buffer is HOST_VISIBLE so the CPU can write to it, and
    // TRANSFER_SRC so Vulkan knows it will be the source of a copy command.
    Buffer staging(m_core);
    staging.create(
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    staging.uploadData(array);

    // ----- Steps 3–5: single-use command buffer copy -------------------------
    // Core::beginSingleTimeCommands() allocates a command buffer from the
    // transfer pool and calls vkBeginCommandBuffer on it.
    // Core::endSingleTimeCommands() submits it to the graphics queue,
    // waits (vkQueueWaitIdle), then frees the command buffer.
    VkCommandBuffer cmd = m_core.beginSigleTimeCommands();
    staging.copyTo(*this, cmd, dataSize);
    m_core.endSingleTimeCommands(cmd);

    // ----- Step 6: staging buffer destroyed here automatically ---------------
    // staging goes out of scope → Buffer::~Buffer() → vkDestroyBuffer +
    // vkFreeMemory. The GPU copy is already complete at this point because
    // endSingleTimeCommands() waited for the queue to be idle.
}
