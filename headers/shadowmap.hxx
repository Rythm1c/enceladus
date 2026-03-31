#ifndef SHADOWMAP_HXX
#define SHADOWMAP_HXX

#include <vulkan/vulkan.h>
#include <memory>
#include "texture.hxx"
#include "../external/math/mat4.hxx"
#include "../external/math/vec3.hxx"

class Core;
class Pipeline;

struct ShadowMapPushConstants{
    Mat4x4 model;
    Mat4x4 lightSpaceMatrix;
};

class ShadowMap{
public:
    /**
     * @param core       Core reference.
     * @param resolution Shadow map size in texels (power of 2, default 2048).
     *                   Higher = sharper shadows but more VRAM and fill cost.
     */
    ShadowMap(Core &core, uint32_t resolution = 2048);

    ShadowMap(const ShadowMap &)            = delete;
    ShadowMap &operator=(const ShadowMap &) = delete;
    ShadowMap(ShadowMap &&)                 = delete;
    ShadowMap &operator=(ShadowMap &&)      = delete;

    ~ShadowMap();

    // ---- Accessors used by Renderer ----------------------------------------
    VkRenderPass  getRenderPass()  const { return m_renderPass;  }
    VkFramebuffer getFramebuffer() const { return m_framebuffer; }
    VkExtent2D    getExtent()      const { return m_extent;      }

    // ---- Accessors used by Descriptor (binding 2) --------------------------
    VkImageView   getView()    const { return m_texture->getView(); }
    VkSampler     getSampler() const { return m_sampler;            }

    Mat4x4 computeLightSpaceMatrix(
        Vector3f lightDir,
        Vector3f sceneCenter = {0.0f, 0.0f, 0.0f},
        float    sceneRadius = 15.0f) const;

    //----Shadow Creation cycle---------------------------------
    //render objects to shadowmap before to the actual screen
    // call beginRenderpass() then draw objects using shadow pass pipeline then finish with endRenderpass 
    // pass current renderer command buffer
    void beginRenderpass(VkCommandBuffer cmd);
    void drawShapeShadow(VkCommandBuffer cmd, const class Shape &shape, const Mat4x4 &lightSpaceMatrix);
    void endRenderpass(VkCommandBuffer cmd);

private:
    Core                    & m_core;
    VkExtent2D                m_extent;
    std::unique_ptr<Texture>  m_texture;
    std::unique_ptr<Pipeline> m_pipeline;
    VkRenderPass              m_renderPass  = VK_NULL_HANDLE;
    VkFramebuffer             m_framebuffer = VK_NULL_HANDLE;
    VkSampler                 m_sampler     = VK_NULL_HANDLE;

    void createRenderPass();
    void createPipeline();
    void createFramebuffer();
    void createSampler();
};

#endif