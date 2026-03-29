#ifndef DESCRIPTOR_HXX
#define DESCRIPTOR_HXX

#include <vulkan/vulkan.h>
#include "buffer.hxx"
#include "ubo.hxx"

class Core;

class Descriptor
{
public:
    explicit Descriptor(Core &core, uint32_t framesInFlight);

    Descriptor(const Descriptor &)            = delete;
    Descriptor &operator=(const Descriptor &) = delete;
    Descriptor(Descriptor &&)                 = delete;
    Descriptor &operator=(Descriptor &&)      = delete;

    ~Descriptor();

    // for pipeline layout creation
    VkDescriptorSetLayout getLayout() const { return m_layout; }
    /**
     * Writes new camera data into the UBO buffer for @p frameIndex.
     * Call this every frame before binding the descriptor set.
     */
    void update(uint32_t frameIndex, const CameraUBO &ubo);

private:

    Core                   &m_core;
    uint32_t                m_framesInFlight;

    VkDescriptorSetLayout   m_layout = VK_NULL_HANDLE;
    VkDescriptorPool        m_pool   = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_sets;       // one per frame
    std::vector<Buffer>          m_uboBuffers; // one per frame

    void createLayout();
    void createPool();
    void createSetsAndBuffers();
};
#endif