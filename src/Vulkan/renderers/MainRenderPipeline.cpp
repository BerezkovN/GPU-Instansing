#include "MainRenderPipeline.hpp"

#include "../App.hpp"
#include "../pch.hpp"
#include "../helpers/buffers/LocalBuffer.hpp"


MainRenderPipeline::MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc) {

    m_app = desc.app;
    m_framesInFlight = desc.framesInFlight;
    m_device = desc.device;
    m_renderPass = desc.renderPass;

    // TODO: Use pUseSpecializationInfo for GPU Instancing variants.
    const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = desc.vertexShader,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = desc.fragmentShader,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

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

    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
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

    this->CreateDescriptorSetLayout();
    // TODO: Learn more about this
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout
    };

    VkResult result = vkCreatePipelineLayout(m_device->GetVkDevice(), & pipelineLayoutCreateInfo, nullptr, & m_pipelineLayout);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create pipeline layout: " + std::to_string(result));
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &blendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = m_pipelineLayout,
        .renderPass = desc.renderPass->GetVkRenderPass(),
        .subpass = 0,
        // TODO: Learn more about this
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(m_device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create graphics pipeline: " + std::to_string(result));
    }

    spdlog::info("[MainRenderPipeline] Graphics pipeline was created successfully!");

    this->CreateDescriptorPool();
    this->CreateDescriptorSets(desc.descriptorSetWrites);
}

void MainRenderPipeline::Destroy() {

    this->DestroyDescriptorPool();
    this->DestroyDescriptorSetLayout();

	vkDestroyPipeline(m_device->GetVkDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->GetVkDevice(), m_pipelineLayout, nullptr);
}

void MainRenderPipeline::BindDescriptors(VkCommandBuffer buffer, uint32_t frameIndex) {
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, DescriptorSetCount, m_descriptorSets[frameIndex].indexed.data(), 0, nullptr);
}

VkPipeline MainRenderPipeline::GetVkPipeline() const {
	return m_pipeline;
}

VkPipelineLayout MainRenderPipeline::GetVkPipelineLayout() const {
    return m_pipelineLayout;
}

void MainRenderPipeline::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        // TODO: Learn more about this
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };


    const VkResult result = vkCreateDescriptorSetLayout(m_device->GetVkDevice(), &createInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create descriptor set layout");
    }
}

void MainRenderPipeline::DestroyDescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(m_device->GetVkDevice(), m_descriptorSetLayout, nullptr);
    m_descriptorSetLayout = VK_NULL_HANDLE;
}


void MainRenderPipeline::CreateDescriptorPool() {

    VkDescriptorPoolSize poolSize = {
        // TODO: Learn more
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = m_framesInFlight * 1
    };

    const VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = m_framesInFlight * DescriptorSetCount,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };

    const VkResult result = vkCreateDescriptorPool(m_device->GetVkDevice(), &poolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create descriptor pool");
    }
}

void MainRenderPipeline::DestroyDescriptorPool() {

    // Should automatically get cleaned when destroying descriptor pool.
	m_descriptorSets.clear();

    vkDestroyDescriptorPool(m_device->GetVkDevice(), m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;

}

void MainRenderPipeline::CreateDescriptorSets(const std::vector<PipelineDescriptorSets>& descriptorSetWrites) {

    m_descriptorSets.resize(descriptorSetWrites.size());

    for (uint32_t ind = 0; ind < m_framesInFlight; ind++) {

        const VkDescriptorSetAllocateInfo allocateInfo = {
		    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		    .descriptorPool = m_descriptorPool,
		    .descriptorSetCount = DescriptorSetCount,
		    .pSetLayouts = &m_descriptorSetLayout
        };

        const VkResult result = vkAllocateDescriptorSets(m_device->GetVkDevice(), &allocateInfo, m_descriptorSets[ind].indexed.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[MainRenderPipeline] Could not allocate descriptor sets");
        }

        VkDescriptorBufferInfo bufferInfo = {
		    .buffer = descriptorSetWrites[ind].uniformBufferObject.buffer,
		    .offset = descriptorSetWrites[ind].uniformBufferObject.offset,
		    .range = descriptorSetWrites[ind].uniformBufferObject.range
        };

        VkWriteDescriptorSet writeDescriptorSet = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[ind].named.uniformBufferObject,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = descriptorSetWrites[ind].uniformBufferObject.descriptorType,
            .pBufferInfo = &bufferInfo
        };

        vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }

}

