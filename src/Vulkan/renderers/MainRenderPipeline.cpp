#include "MainRenderPipeline.hpp"

#include "../pch.hpp"
#include "../helpers/buffers/StagingBuffer.hpp"

struct Vertex
{
    glm::vec3 position;
    int color;
};

MainRenderPipeline::MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc) {

    m_framesInFlight = desc.framesInFlight;
    m_device = desc.device;
    m_renderPass = desc.renderPass;
    m_graphicsQueue = desc.graphicsQueue;
    m_transferQueue = desc.transferQueue;

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
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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

    this->CreateCommandPools();
    this->CreateCommandBuffers();
    this->CreateVertexBuffer();
}

void MainRenderPipeline::Destroy() {

    this->DestroyVertexBuffer();
    this->DestroyCommandBuffers();
    this->DestroyCommandPools();

	vkDestroyPipeline(m_device->GetVkDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->GetVkDevice(), m_pipelineLayout, nullptr);
}

void MainRenderPipeline::RecordAndSubmit(const MainRenderPipeline::RecordDesc& desc) const {

    VkCommandBuffer commandBuffer = m_graphicsCommandBuffers[desc.frameIndex];

    vkResetCommandBuffer(commandBuffer, 0);

    constexpr VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        // TODO: Learn about this
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not begin command buffer: " + std::to_string(result));
    }

    VkClearValue clearValue = {
        .color = {.float32 = { 0.0f, 0.0f, 0.0f, 1.0f } }
    };


    const VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass->GetVkRenderPass(),
        .framebuffer = desc.framebuffer,
        .renderArea = desc.renderArea,
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport = {
        .x = 0, .y = 0,
        .width = static_cast<float>(desc.renderArea.extent.width),
        .height = static_cast<float>(desc.renderArea.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = desc.renderArea.extent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not end command buffer: " + std::to_string(result));
    }

    VkSemaphore waitSemaphores[] = { desc.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphore[] = { desc.renderFinishedSemaphore };

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphore
    };


    result = vkQueueSubmit(m_graphicsQueue->GetVkQueue(), 1, &submitInfo, desc.submitFrameFence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Error submitting a graphics queue: " + std::to_string(result));
    }

}


VkPipeline MainRenderPipeline::GetVkPipeline() const {
	return m_pipeline;
}

void MainRenderPipeline::CreateCommandPools() {
    const VkCommandPoolCreateInfo graphicsPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = m_graphicsQueue->GetFamilyIndex()
    };

    VkResult result = vkCreateCommandPool(m_device->GetVkDevice(), &graphicsPoolCreateInfo, nullptr, &m_graphicsCommandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create a graphics command pool: " + std::to_string(result));
    }

    if (!m_transferQueue.has_value()) {
        return;
    }

    const VkCommandPoolCreateInfo transferPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_transferQueue.value()->GetFamilyIndex()
    };

    VkCommandPool transferCommandPool;
    result = vkCreateCommandPool(m_device->GetVkDevice(), &transferPoolCreateInfo, nullptr, &transferCommandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create a transfer command pool: " + std::to_string(result));
    }

    m_transferCommandPool = transferCommandPool;
}

void MainRenderPipeline::DestroyCommandPools() {

    if (m_transferCommandPool.has_value()) {
        vkDestroyCommandPool(m_device->GetVkDevice(), m_transferCommandPool.value(), nullptr);
    }
    vkDestroyCommandPool(m_device->GetVkDevice(), m_graphicsCommandPool, nullptr);

    m_transferCommandPool = VK_NULL_HANDLE;
    m_graphicsCommandPool = VK_NULL_HANDLE;
}

void MainRenderPipeline::CreateCommandBuffers() {
    m_graphicsCommandBuffers.resize(m_framesInFlight);

    const VkCommandBufferAllocateInfo graphicsBuffersInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = m_framesInFlight
    };

    VkResult result = vkAllocateCommandBuffers(m_device->GetVkDevice(), &graphicsBuffersInfo, m_graphicsCommandBuffers.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not allocate graphics command buffers: " + std::to_string(result));
    }

    const VkCommandBufferAllocateInfo transferBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_transferCommandPool.has_value() ? m_transferCommandPool.value() : m_graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    result = vkAllocateCommandBuffers(m_device->GetVkDevice(), &transferBufferInfo, &m_transferCommandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not allocate transfer command buffer: " + std::to_string(result));
    }
}

void MainRenderPipeline::DestroyCommandBuffers() {

    vkFreeCommandBuffers(m_device->GetVkDevice(), 
        m_transferCommandPool.has_value() ? m_transferCommandPool.value() : m_graphicsCommandPool, 
        1, &m_transferCommandBuffer);

    vkFreeCommandBuffers(m_device->GetVkDevice(), m_graphicsCommandPool,
        static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());

    m_graphicsCommandBuffers.clear();
    m_transferCommandBuffer = VK_NULL_HANDLE;

}

void MainRenderPipeline::CreateVertexBuffer() {
    const std::vector<Vertex> vertices = {
	    {{-0.5f, 0.5f, 0.0f}, -1},
	    {{ 0.5f, 0.5f, 0.0f}, 0xFF03102},
	    {{ 0.0f,  -0.5f, 0.0f}, -1},
    };

    const uint32_t bufferSize = sizeof(Vertex) * static_cast<uint32_t>(vertices.size());

    StagingBuffer::Desc stagingBufferDesc = {
        .graphicsQueue = m_graphicsQueue,
        .transferQueue = m_transferQueue,
        .bufferSize = bufferSize
    };
    const auto stagingBuffer = std::make_unique<StagingBuffer>(m_device, stagingBufferDesc);
    stagingBuffer->CopyData(vertices.data(), bufferSize);

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	        .size = sizeof(Vertex) * vertices.size(),
	        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        },
        .memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    m_vertexBuffer = std::make_unique<GenericBuffer>(m_device, desc);
    m_vertexBuffer->CopyFromBuffer(m_transferCommandBuffer, stagingBuffer.get(), {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = bufferSize
    });

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_transferCommandBuffer
    };

    const VkQueue copyQueue = m_transferQueue.has_value() ? m_transferQueue.value()->GetVkQueue() : m_graphicsQueue->GetVkQueue();
    vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(copyQueue);

    stagingBuffer->Destroy();
}

void MainRenderPipeline::DestroyVertexBuffer() {
    m_vertexBuffer->Destroy();
    m_vertexBuffer = nullptr;
}

