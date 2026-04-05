#include "headers/shadowmap.hxx"
#include "headers/pipeline.hxx"
#include "headers/shader.hxx"
#include "headers/drawable.hxx"
#include "headers/vertex.hxx"
#include "headers/core.hxx"

#include <stdexcept>
#include <array>

struct ShadowMapPushConstants{
    Mat4x4 model;
    Mat4x4 lightSpaceMatrix;
};

ShadowMap::ShadowMap(Core &core, uint32_t resolution)
    : m_core(core),
      m_extent{resolution, resolution}
{
    // Depth texture -- same format as the depth buffer, OPTIMAL tiling,
    // SAMPLED usage added so the fragment shader can read it in the main pass.
    TextureConfig cfg{};
    cfg.extent           = m_extent;
    cfg.format           = VK_FORMAT_D32_SFLOAT;
    cfg.usage            = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    cfg.aspectMask       = VK_IMAGE_ASPECT_DEPTH_BIT;
    cfg.tiling           = VK_IMAGE_TILING_OPTIMAL;
    cfg.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    m_texture = std::make_unique<Texture>(core, cfg);

    createRenderPass();
    createPipeline();
    createFramebuffer();
    createSampler();
}

ShadowMap::~ShadowMap()
{
    VkDevice device = m_core.getDevice();
    vkDestroySampler    (device, m_sampler,     nullptr);
    vkDestroyFramebuffer(device, m_framebuffer, nullptr);
    vkDestroyRenderPass (device, m_renderPass,  nullptr);
    // m_texture destroyed via unique_ptr
}

void ShadowMap::createRenderPass()
{
    // Single depth attachment, no colour.
    // finalLayout = SHADER_READ_ONLY_OPTIMAL so it transitions automatically
    // into the layout the fragment shader expects when it samples from it.
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // must STORE -- read in main pass
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 0;       // no colour output
    subpass.pDepthStencilAttachment = &depthRef;

    // Two dependencies:
    // 1. Ensure previous shadow pass finishes before we start writing depth again.
    // 2. Ensure shadow pass depth writes are visible to the fragment shader
    //    in the subsequent main pass (SHADER_READ is the dst access).
    std::array<VkSubpassDependency, 2> deps{};

    deps[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass      = 0;
    deps[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    deps[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    deps[1].srcSubpass      = 0;
    deps[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &depthAttachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
    rpInfo.pDependencies   = deps.data();

    if (vkCreateRenderPass(m_core.getDevice(), &rpInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: vkCreateRenderPass failed!");
}

void ShadowMap::createPipeline(){
    // Shadow pipeline -- depth-only, uses shadow map's render pass and extent,
    // only needs the vertex shader (depth written automatically, no frag output)
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset     = 0;
    pushRange.size       = sizeof(ShadowMapPushConstants);

    auto attribDescs = Vertex3D::getAttributeDescriptions();

    Shader shadowVertShader(m_core, "build/shaders/shadow.vert.spv");
    PipelineConfig shadowConfig{
        .core                  = m_core,
        .renderPass            = m_renderPass,
        .swapChainExtent       = m_extent,
        .vertShader            = &shadowVertShader,
        .fragShader            = nullptr,
        .globalDescLayout      = VK_NULL_HANDLE,
        .materialDescLayout    = VK_NULL_HANDLE,
        .depthOnly             = true,
        .bindingDescriptions   = {Vertex3D::getBindingDescription()},
        .attributeDescriptions = {attribDescs.begin(), attribDescs.end()},
        .pushConstantRanges    = {pushRange},
        // No descriptor layout for shadow pass -- only model push constant needed.
        // The shadow vertex shader only reads push.model and the lightSpaceMatrix
        // which is passed as a separate push constant range (see shadow.vert).
        
    };

    m_pipeline = std::make_unique<Pipeline>(shadowConfig);
}

void ShadowMap::createFramebuffer()
{
    VkImageView view = m_texture->getView();

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass      = m_renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments    = &view;
    fbInfo.width           = m_extent.width;
    fbInfo.height          = m_extent.height;
    fbInfo.layers          = 1;

    if (vkCreateFramebuffer(m_core.getDevice(), &fbInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: vkCreateFramebuffer failed!");
}

void ShadowMap::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // LINEAR filter enables the hardware to interpolate between shadow map
    // texels, which is required for PCF to work correctly.
    samplerInfo.magFilter    = VK_FILTER_LINEAR;
    samplerInfo.minFilter    = VK_FILTER_LINEAR;

    // CLAMP_TO_BORDER with white border: fragments outside the shadow frustum
    // get depth = 1.0 (fully lit) instead of wrapping or clamping to an edge.
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    // Depth comparison: texture() returns 1.0 if stored_depth >= ref_depth
    // (fragment is lit), 0.0 if stored_depth < ref_depth (fragment is shadowed).
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;

    samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias   = 0.0f;
    samplerInfo.minLod       = 0.0f;
    samplerInfo.maxLod       = 1.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;

    if (vkCreateSampler(m_core.getDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: vkCreateSampler failed!");
}

Mat4x4 ShadowMap::computeLightSpaceMatrix(
    Vector3f lightDir,
    Vector3f sceneCenter,
    float    sceneRadius) const
{
    // Place the light source far enough back along its direction that the
    // entire scene sphere fits between the near and far planes.
    const Vector3f lightPos = sceneCenter - lightDir.unit() * sceneRadius;

    // lookAt: light looks toward the scene centre from lightPos.
    // Choose up = world Y unless light direction is nearly vertical,
    // in which case use world Z to avoid degenerate cross product.
    const Vector3f up = (std::abs(lightDir.unit().y) < 0.99f) ? Vector3f(0.0f, 1.0f, 0.0f) : Vector3f(0.0f, 0.0f, 1.0f);

    const Mat4x4 lightView = look_at(lightPos, sceneCenter, up);

    // Orthographic projection -- all rays are parallel for a directional light.
    // The frustum is a box centred on sceneCenter with half-size = sceneRadius.
    // near is negative to include objects behind the light camera for proper shadow coverage
    const Mat4x4 lightProj = orthogonal(
        -sceneRadius,  sceneRadius,   // left, right
        -sceneRadius,  sceneRadius,   // bottom, top
        -sceneRadius,       sceneRadius * 2.0f); // near, far (near is negative to extend behind camera)

    // Vulkan Y-flip (same correction as the camera)
    Mat4x4 proj = lightProj;
    proj.yy *= -1.0f;
    //vulkan expects a column major mat hence the transpose()
    return (proj * lightView).transpose();
}

void ShadowMap::beginRenderpass(VkCommandBuffer cmd)
{
    // Clear depth to 1.0 (far plane) -- fragments closer than this will write
    VkClearValue depthClear{};
    depthClear.depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass        = m_renderPass;
    rpInfo.framebuffer       = m_framebuffer;
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = m_extent;
    rpInfo.clearValueCount   = 1;
    rpInfo.pClearValues      = &depthClear;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float) m_extent.width;
    viewport.height   = (float) m_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getHandle());
}

void ShadowMap::drawShadow(VkCommandBuffer cmd, const Drawable &drawable, const Mat4x4 &lightSpaceMatrix)
{
    ShadowMapPushConstants pushConstants; 
    pushConstants.model            = drawable.model;
    pushConstants.lightSpaceMatrix = lightSpaceMatrix;

    vkCmdPushConstants(
        cmd,
        m_pipeline->getLayout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(ShadowMapPushConstants),
        &pushConstants);

    // Bind the vertex buffer
    VkBuffer buffers[] = {drawable.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    if(drawable.indexCount > 0)
    {
        vkCmdBindIndexBuffer(cmd, drawable.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(drawable.indexCount), 1, 0, 0, 0);
    }
    else
    {
    vkCmdDraw(cmd, static_cast<uint32_t>(drawable.vertexCount), 1, 0, 0);
    }
}
void ShadowMap::endRenderpass(VkCommandBuffer cmd)
{
    vkCmdEndRenderPass(cmd);
}