template <typename T>
void Buffer::uploadData(const std::vector<T> &data)
{
    VkDeviceSize dataSize = sizeof(T) * array.size();
    assert(dataSize <= this->size && "buffer size too small for upload data!");
    void *mapped;
    vkMapMemory(ref_core.getDevice(), bufferMemory, 0, dataSize, 0, &mapped);
    memcpy(mapped, array.data(), static_cast<size_t>(dataSize));
    vkUnmapMemory(ref_core.getDevice(), bufferMemory);
}