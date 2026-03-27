#include "core.hxx"
template <typename T>
void Buffer::uploadData(const std::vector<T> &array)
{
    VkDeviceSize dataSize = sizeof(T) * array.size();
    //assert(dataSize <= this->m_size && "buffer size too small for upload data!");
    void *mapped;
    vkMapMemory(ref_core.getDevice(), m_bufferMemory, 0, dataSize, 0, &mapped);
    memcpy(mapped, array.data(), static_cast<size_t>(dataSize));
    vkUnmapMemory(ref_core.getDevice(), m_bufferMemory);
}

template<typename T>
Buffer createDeviceLocalBuffer(
    Core& core,
    const std::vector<T>& data,
    VkBufferUsageFlags usage)
{
    VkDeviceSize dataSize =
        sizeof(T) * data.size();

    // --- staging buffer ---
    Buffer staging(core);

    staging.create(
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    staging.uploadData(data);

    // --- device buffer ---
    Buffer deviceBuffer(core);

    deviceBuffer.create(
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // --- copy ---
    copyBuffer(
        core,
        staging.get(),
        deviceBuffer.get(),
        dataSize);

    return deviceBuffer;

}