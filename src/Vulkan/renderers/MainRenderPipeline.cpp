#include "MainRenderPipeline.hpp"

#include "../App.hpp"
#include "../pch.hpp"


MainRenderPipeline::MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc) {

    m_app = desc.app;
    m_device = desc.device;
    m_renderPass = desc.renderPass;
    m_shaderLayout = desc.shaderLayout;
    
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkVertexInputBindingDescription bindingDescription = {
	    .binding = 0,
	    .stride = sizeof(Vertex),
	    // TODO: Instancing
	    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    const auto attributeDescriptions = this->GetAttributes();

    const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .vertexBindingDescriptionCount = 1,
	    .pVertexBindingDescriptions = &bindingDescription,
	    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
	    .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Viewport is a dynamic state and updated later.
    VkPipelineViewportStateCreateInfo viewportCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    // TODO: Implement depth.
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    VkPipelineColorBlendAttachmentState blendAttachmentState = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentState,
    };

    std::vector<VkPipelineShaderStageCreateInfo> stages = m_shaderLayout->GetVkShaderStages();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInputStateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &blendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = m_shaderLayout->GetVkPipelineLayout(),
        .renderPass = desc.renderPass->GetVkRenderPass(),
        .subpass = 0,
        // TODO: Learn more about this
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    const VkResult result = vkCreateGraphicsPipelines(m_device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create graphics pipeline: " + std::to_string(result));
    }

    spdlog::info("[MainRenderPipeline] Graphics pipeline was created successfully!");
}

void MainRenderPipeline::Destroy() {

	vkDestroyPipeline(m_device->GetVkDevice(), m_pipeline, nullptr);
}

VkPipeline MainRenderPipeline::GetVkPipeline() const {
	return m_pipeline;
}

std::vector<VkVertexInputAttributeDescription> MainRenderPipeline::GetAttributes() {

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.push_back({
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = 0
    });

    attributeDescriptions.push_back({
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .offset = offsetof(Vertex, color)
    });

    return attributeDescriptions;
}

