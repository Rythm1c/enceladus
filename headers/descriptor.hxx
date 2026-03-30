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

    VkDescriptorSet getSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    /**
     * Writes new camera data into the UBO buffer for @p frameIndex.
     * Call this every frame before binding the descriptor set.
     */
    // Update camera UBO for the given frame (call before binding)
    void updateCamera(uint32_t frameIndex, const CameraUBO &camera);

    // Update light UBO for the given frame (call before binding)
    void updateLight(uint32_t frameIndex, const LightUBO &light);

private:

    Core                   &m_core;
    uint32_t                m_framesInFlight;

    VkDescriptorSetLayout   m_layout = VK_NULL_HANDLE;
    VkDescriptorPool        m_pool   = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_sets;       // one per frame
    // Two buffers per frame -- one for camera, one for light
    std::vector<Buffer>          m_cameraBuffers;
    std::vector<Buffer>          m_lightBuffers;

    void createLayout();
    void createPool();
    void createSetsAndBuffers();
};
#endif