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