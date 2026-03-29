#include "../headers/descriptor.hxx"
#include "../headers/core.hxx"

#include <stdexcept>

Descriptor::Descriptor(Core &core, uint32_t framesInFlight)
    : m_core(core),
      m_framesInFlight(framesInFlight)
{
    // Order matters: layout → pool → sets+buffers
    createLayout();
    createPool();
    createSetsAndBuffers();
}

Descriptor::~Descriptor()
{
    VkDevice device = m_core.getDevice();

    // Descriptor sets are implicitly freed when the pool is destroyed —
    // no need to free them individually.
    vkDestroyDescriptorPool(device, m_pool, nullptr);

    // Layout must outlive the pipeline that was created with it.
    // Since Descriptor is destroyed after Pipeline in main (reverse declaration
    // order), this is safe.
    vkDestroyDescriptorSetLayout(device, m_layout, nullptr);

    // m_uboBuffers destroy themselves via Buffer::~Buffer()
}

void Descriptor::createLayout()
{
    /**
     * A VkDescriptorSetLayoutBinding describes one "slot" in the shader.
     *
     * binding = 0           → matches `layout(binding = 0)` in the GLSL
     * descriptorType        → UNIFORM_BUFFER because it's a UBO
     * descriptorCount = 1   → one UBO (not an array)
     * stageFlags            → only the vertex shader needs view/proj
     */
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding            = 0;
    uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount    = 1;
    uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboBinding.pImmutableSamplers = nullptr; // only used for texture samplers

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &uboBinding;

    if (vkCreateDescriptorSetLayout(m_core.getDevice(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("Descriptor: vkCreateDescriptorSetLayout failed!");
}

void Descriptor::createPool()
{
    /**
     * The pool pre-allocates capacity for descriptor sets.
     *
     * VkDescriptorPoolSize says: "I need space for N descriptors of type T."
     * We have one UBO binding per frame, so the count = framesInFlight.
     *
     * maxSets is the maximum number of VkDescriptorSets that can be allocated
     * from this pool in total — also framesInFlight here.
     */
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = m_framesInFlight;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = m_framesInFlight;

    if (vkCreateDescriptorPool(m_core.getDevice(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("Descriptor: vkCreateDescriptorPool failed!");
}

void Descriptor::createSetsAndBuffers()
{
    VkDevice device = m_core.getDevice();

    // Allocate all descriptor sets in one call.
    // Each set gets the same layout (they all describe the same binding schema).
    std::vector<VkDescriptorSetLayout> layouts(m_framesInFlight, m_layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_pool;
    allocInfo.descriptorSetCount = m_framesInFlight;
    allocInfo.pSetLayouts        = layouts.data();

    m_sets.resize(m_framesInFlight);
    if (vkAllocateDescriptorSets(device, &allocInfo, m_sets.data()) != VK_SUCCESS)
        throw std::runtime_error("Descriptor: vkAllocateDescriptorSets failed!");

    // Create one UBO buffer per frame and immediately point the descriptor
    // set at it.  The buffer contents are updated each frame via update().
    m_uboBuffers.reserve(m_framesInFlight);
    for (uint32_t i = 0; i < m_framesInFlight; ++i)
    {
        // Emplace constructs Buffer in-place (Buffer is non-copyable)
        m_uboBuffers.emplace_back(m_core);
        m_uboBuffers[i].create(
            sizeof(CameraUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            // HOST_VISIBLE  → CPU can write to it via vkMapMemory
            // HOST_COHERENT → writes are immediately visible to the GPU,
            //                 no explicit flush needed
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Keep the buffer persistently mapped — avoids map/unmap every frame
        m_uboBuffers[i].mapPersistent();

        /**
         * vkUpdateDescriptorSets "wires" the descriptor set to the buffer.
         *
         * VkDescriptorBufferInfo tells Vulkan which buffer, at what offset,
         * and how many bytes the shader should see.
         *
         * VkWriteDescriptorSet is the update command itself:
         *   dstSet     → which descriptor set to update
         *   dstBinding → which binding slot (must match the layout)
         *   dstArrayElement → 0 (not an array)
         *   descriptorType  → must match the layout binding
         */
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uboBuffers[i].get();
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(CameraUBO);

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_sets[i];
        write.dstBinding      = 0;
        write.dstArrayElement = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo     = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

// =============================================================================
// Per-frame update
// =============================================================================

void Descriptor::update(uint32_t frameIndex, const CameraUBO &ubo)
{
    // The buffer is persistently mapped so this is just a memcpy —
    // no Vulkan calls needed, and HOST_COHERENT means no flush required.
    m_uboBuffers[frameIndex].uploadData(
        std::vector<CameraUBO>{ubo});
}
