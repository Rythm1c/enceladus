#include "../headers/descriptor.hxx"
#include "../headers/shadowmap.hxx"
#include "../headers/core.hxx"

#include <stdexcept>
#include <array>

Descriptor::Descriptor(Core &core, uint32_t framesInFlight, const ShadowMap &shadowMap)
    : m_core(core),
      m_framesInFlight(framesInFlight),
      m_shadowMap(shadowMap)
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
    // Binding 0 -- CameraUBO (vertex stage only, view/proj not needed in frag)
    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding            = 0;
    cameraBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBinding.descriptorCount    = 1;
    cameraBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    cameraBinding.pImmutableSamplers = nullptr;

    // Binding 1 -- LightUBO (fragment stage -- lighting is computed per fragment)
    // Also include vertex bit in case we later need light direction for
    // vertex-level effects like shadow map projection.
    VkDescriptorSetLayoutBinding lightBinding{};
    lightBinding.binding            = 1;
    lightBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightBinding.descriptorCount    = 1;
    lightBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT
                                    | VK_SHADER_STAGE_VERTEX_BIT;
    lightBinding.pImmutableSamplers = nullptr;

    // binding 2 -- shadow map (combined image sampler)
    // COMBINED_IMAGE_SAMPLER bundles the VkImageView and VkSampler into one
    // binding. This is the standard way to expose a texture to a shader.
    VkDescriptorSetLayoutBinding shadowBinding{};
    shadowBinding.binding         = 2;
    shadowBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowBinding.descriptorCount = 1;
    shadowBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {cameraBinding, lightBinding, shadowBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

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
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = m_framesInFlight * 2; // camera + light
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = m_framesInFlight; // light buffers

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
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
    m_cameraBuffers.reserve(m_framesInFlight);
    m_lightBuffers.reserve(m_framesInFlight);    
    for (uint32_t i = 0; i < m_framesInFlight; ++i)
    {
        // Camera buffer
        m_cameraBuffers.emplace_back(m_core);
        m_cameraBuffers[i].create(
            sizeof(CameraUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_cameraBuffers[i].mapPersistent();

        // Light buffer
        m_lightBuffers.emplace_back(m_core);
        m_lightBuffers[i].create(
            sizeof(LightUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_lightBuffers[i].mapPersistent();


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
        VkDescriptorBufferInfo cameraInfo{};
        cameraInfo.buffer = m_cameraBuffers[i].get();
        cameraInfo.offset = 0;
        cameraInfo.range  = sizeof(CameraUBO);

        VkDescriptorBufferInfo lightInfo{};
        lightInfo.buffer = m_lightBuffers[i].get();
        lightInfo.offset = 0;
        lightInfo.range  = sizeof(LightUBO);

        VkDescriptorImageInfo shadowInfo{};
        shadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowInfo.imageView   = m_shadowMap.getView();
        shadowInfo.sampler     = m_shadowMap.getSampler();

        std::array<VkWriteDescriptorSet, 3> writes{};

        writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet          = m_sets[i];
        writes[0].dstBinding      = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo     = &cameraInfo;

        writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet          = m_sets[i];
        writes[1].dstBinding      = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo     = &lightInfo;

        writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet          = m_sets[i];
        writes[2].dstBinding      = 2;
        writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[2].descriptorCount = 1;
        writes[2].pImageInfo      = &shadowInfo;

        vkUpdateDescriptorSets(device,
            static_cast<uint32_t>(writes.size()), writes.data(),
            0, nullptr);
    }
}

// =============================================================================
// Per-frame update
// =============================================================================

void Descriptor::updateCamera(uint32_t frameIndex, const CameraUBO &camera)
{
    m_cameraBuffers[frameIndex].uploadData(std::vector<CameraUBO>{camera});
}

void Descriptor::updateLight(uint32_t frameIndex, const LightUBO &light)
{
    m_lightBuffers[frameIndex].uploadData(std::vector<LightUBO>{light});
}
