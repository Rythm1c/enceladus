#include "headers/pipeline.hxx"
#include "headers/core.hxx"

#include <fstream>
#include <vector>
#include <string>
#include <iostream>

Pipeline::Pipeline(PipelineConfig &config)
    : m_core(config.core)
{

    VkDevice device = m_core.getDevice();

    if (!config.vertShader)
    {
        throw std::runtime_error("Vertex shaders must be provided!");
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = config.vertShader->getHandle();
    vertShaderStageInfo.pName  = "main";
    shaderStages.push_back(vertShaderStageInfo);


    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    if(!config.depthOnly && config.fragShader)
    {
        fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = config.fragShader->getHandle();
        fragShaderStageInfo.pName  = "main";
        shaderStages.push_back(fragShaderStageInfo);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(config.bindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions      = config.bindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = config.attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = config.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    /* VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)config.swapChainExtent.width;
    viewport.height   = (float)config.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = config.swapChainExtent; */

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    //viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    //viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = config.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    if(config.depthOnly)
    {
        // Cull FRONT faces during shadow pass.
        // This ensures only back faces write to the shadow map, which eliminates
        // "peter panning" -- the artifact where shadows appear detached from objects
        // because the surface normal and shadow ray are nearly parallel.
        rasterizer.cullMode  = VK_CULL_MODE_FRONT_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // Depth bias: nudges depth values slightly to prevent shadow acne
        // (self-shadowing noise caused by floating-point precision limits).
        rasterizer.depthBiasEnable         = VK_TRUE;
        rasterizer.depthBiasConstantFactor = 4.0f;  // constant offset
        rasterizer.depthBiasSlopeFactor    = 2.5f;  // slope-dependent offset

    } else
    {
        rasterizer.cullMode        = config.wireframe ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
    }
    

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    const bool hasDescriptorLayout = (config.descriptorSetLayout != VK_NULL_HANDLE);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = hasDescriptorLayout ? 1 : 0;
    layoutInfo.pSetLayouts            = hasDescriptorLayout ? &config.descriptorSetLayout : nullptr;
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size());
    layoutInfo.pPushConstantRanges    = config.pushConstantRanges.empty() ? nullptr : config.pushConstantRanges.data();

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Depth testing -- fragments closer to the camera (lower depth value) win.
    // depthWriteEnable must also be true or the depth buffer never gets updated.
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS; // closer fragment wins
    depthStencil.depthBoundsTestEnable = VK_FALSE;           // no min/max depth clamping
    depthStencil.stencilTestEnable     = VK_FALSE;           // no stencil yet

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil; // Optional
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = m_layout;
    pipelineInfo.renderPass          = config.renderPass;
    pipelineInfo.subpass             = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    std::cout << "Pipeline created successfully" << std::endl;
}

Pipeline::~Pipeline()
{
    VkDevice device = m_core.getDevice();
    vkDestroyPipeline(device, m_handle, nullptr);
    vkDestroyPipelineLayout(device, m_layout, nullptr);
}
