#ifndef DESCRIPTORS_HXX
#define DESCRIPTORS_HXX

#include <vulkan/vulkan.h>
#include "buffer.hxx"
#include "ubo.hxx"

class Core;
class ShadowMap;

class GlobalDescriptor
{
public:
    explicit GlobalDescriptor(Core &core, uint32_t framesInFlight, const ShadowMap &shadowMap);

    GlobalDescriptor(const GlobalDescriptor &)            = delete;
    GlobalDescriptor &operator=(const GlobalDescriptor &) = delete;
    GlobalDescriptor(GlobalDescriptor &&)                 = delete;
    GlobalDescriptor &operator=(GlobalDescriptor &&)      = delete;

    ~GlobalDescriptor();

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
    const ShadowMap        &m_shadowMap;

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
// material doesn't change per frame, so we only need one buffer and one set
// might change this later if i support animations or editable materials
class MaterialDescriptor
{
public:
    MaterialDescriptor(Core &core);

    MaterialDescriptor(const MaterialDescriptor &)            = delete;
    MaterialDescriptor &operator=(const MaterialDescriptor &) = delete;
    MaterialDescriptor(MaterialDescriptor &&)                 = delete;
    MaterialDescriptor &operator=(MaterialDescriptor &&)      = delete;

    ~MaterialDescriptor();

    static void createLayout(VkDevice device);
    static VkDescriptorSetLayout getLayout()  { return s_layout; }
    static void destroyLayout(VkDevice device) { if (s_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, s_layout, nullptr); }

    VkDescriptorSet getSet() const { return m_set; }

    void update(const MaterialUBO &material);
private:
    Core                        &m_core;

    static VkDescriptorSetLayout s_layout;
    VkDescriptorPool             m_pool   = VK_NULL_HANDLE;
    VkDescriptorSet              m_set    = VK_NULL_HANDLE;
    Buffer                       m_materialBuffer;

    void createPool();
    void createSetAndBuffer();
};
#endif