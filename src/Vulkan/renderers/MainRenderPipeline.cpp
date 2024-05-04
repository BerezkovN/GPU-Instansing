#include "MainRenderPipeline.hpp"

#include "../pch.hpp"

struct Vertex
{
    glm::vec3 position;
    int color;
};

MainRenderPipeline::MainRenderPipeline(const Device* device, const IRenderPass* renderPass, VkShaderModule fragModule, VkShaderModule vertModule) {

    m_device = device;

    // TODO: Use pUseSpecializationInfo for GPU Instancing variants.
    const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertModule,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragModule,
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
        // What the hell is this?
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

    // TODO: Learn more about this
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
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
        .renderPass = renderPass->GetVkRenderPass(),
        .subpass = 0,
        // TODO: Learn more about this
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create graphics pipeline: " + std::to_string(result));
    }

    spdlog::info("[MainRenderPipeline] Graphics pipeline was created successfully!");

    this->CreateVertexBuffer();
}

void MainRenderPipeline::Destroy() {

    vkDestroyBuffer(m_device->GetVkDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(m_device->GetVkDevice(), m_vertexBufferMemory, nullptr);

	vkDestroyPipeline(m_device->GetVkDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->GetVkDevice(), m_pipelineLayout, nullptr);

    vkDestroyBuffer(m_device->GetVkDevice(), m_vertexBuffer, nullptr);
}

void MainRenderPipeline::BeforeRender(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

VkPipeline MainRenderPipeline::GetVkPipeline() const {
	return m_pipeline;
}

void MainRenderPipeline::CreateVertexBuffer() {

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, -1},
        {{ 0.5f, -0.5f, 0.0f}, 0xFF03102},
        {{ 0.0f,  0.5f, 0.0f}, -1},
    };

    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(Vertex) * vertices.size(),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkResult result = vkCreateBuffer(m_device->GetVkDevice(), &bufferInfo, nullptr, &m_vertexBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not create a vertex buffer");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_device->GetVkDevice(), m_vertexBuffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        // TODO: Learn more about this
        .memoryTypeIndex = this->FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    result = vkAllocateMemory(m_device->GetVkDevice(), &memoryAllocateInfo, nullptr, &m_vertexBufferMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not allocate memory for the vertex buffer");
    }

    vkBindBufferMemory(m_device->GetVkDevice(), m_vertexBuffer, m_vertexBufferMemory, 0);

    void* data;
    vkMapMemory(m_device->GetVkDevice(), m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    std::memcpy(data, vertices.data(), bufferInfo.size);
    vkUnmapMemory(m_device->GetVkDevice(), m_vertexBufferMemory);

}

uint32_t MainRenderPipeline::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device->GetVkPhysicalDevice(), &memoryProperties);

    for (uint32_t ind = 0; ind < memoryProperties.memoryTypeCount; ind++) {
	    if (typeFilter & (1 << ind) && (memoryProperties.memoryTypes[ind].propertyFlags & properties)) {
            return ind;
	    }
    }

    throw std::runtime_error("[MainRenderPipeline] Could not find correct memory type");
}
