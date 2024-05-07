#include "MainRenderPipeline.hpp"

#include "../App.hpp"
#include "../pch.hpp"
#include "../helpers/buffers/LocalBuffer.hpp"

struct Vertex
{
    glm::vec3 position;
    int color;
};

// Beware of alignment!
struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

MainRenderPipeline::MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc) {

    m_app = desc.app;
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

    this->CreateCommandPools();
    this->CreateCommandBuffers();
    this->CreateVertexBuffer();
    this->CreateIndexBuffer();
    this->CreateUniformBuffers();
    this->CreateDescriptorPool();
    this->CreateDescriptorSets();
}

void MainRenderPipeline::Destroy() {

    this->DestroyDescriptorPool();
    this->DestroyDescriptorSetLayout();
    this->DestroyUniformBuffers();
    this->DestroyIndexBuffer();
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

    this->UpdateUniformBuffers(desc.frameIndex);

    // Learn about vkCmdBindVertexBuffers
    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[desc.frameIndex], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

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

void MainRenderPipeline::CreateUniformBuffers() {

    for (uint32_t ind = 0; ind < m_framesInFlight; ind++) {

        GenericBuffer::Desc desc = {
            .bufferCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = sizeof(UniformBufferObject),
                .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            },
            .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        auto uniformBuffer = std::make_unique<GenericBuffer>(m_device, desc);
        uniformBuffer->MapMemory(uniformBuffer->GetBufferSize());
        m_uniformBuffers.push_back(std::move(uniformBuffer));
    }

}

void MainRenderPipeline::UpdateUniformBuffers(uint32_t currentImage) const {

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    int width, height;
    m_app->GetScreenSize(width, height);

    UniformBufferObject ubo = {
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), width / static_cast<float>(height), 0.1f, 10.0f)
    };

    ubo.proj[1][1] *= -1;

    // TODO: Learn about push constants
    void* mappedUboPtr = m_uniformBuffers[currentImage]->GetMappedMemory();
    std::memcpy(mappedUboPtr, &ubo, sizeof(ubo));
}

void MainRenderPipeline::DestroyUniformBuffers() {

    for (const auto& uniformBuffer : m_uniformBuffers) {
	    uniformBuffer->Destroy();
    }

    m_uniformBuffers.clear();
}

void MainRenderPipeline::CreateDescriptorPool() {

    VkDescriptorPoolSize poolSize = {
        // TODO: Learn more
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = m_framesInFlight
    };

    const VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = m_framesInFlight,
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

void MainRenderPipeline::CreateDescriptorSets() {

    std::vector<VkDescriptorSetLayout> layouts(m_framesInFlight, m_descriptorSetLayout);

    const VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = m_framesInFlight,
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets.resize(m_framesInFlight);
    const VkResult result = vkAllocateDescriptorSets(m_device->GetVkDevice(), &allocateInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPipeline] Could not allocate descriptor sets");
    }

    for (uint32_t ind = 0; ind < m_framesInFlight; ind++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = m_uniformBuffers[ind]->GetVkBuffer(),
            .offset = 0,
            .range = m_uniformBuffers[ind]->GetBufferSize()
        };

        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[ind],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo
        };
        vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
    }

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
	    {{-0.5f, -0.5f, 0.0f}, -1},
	    {{-0.5f,  0.5f, 0.0f}, 0xFF03102},
	    {{ 0.5f,  0.5f, 0.0f}, -1},
	    {{ 0.5f, -0.5f, 0.0f}, 0xFF03102},
    };

    LocalBuffer::Desc desc = {
        .graphicsQueue = m_graphicsQueue,
        .transferQueue = m_transferQueue,
        .transferCommandBuffer = m_transferCommandBuffer,
        .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .buffer = vertices.data(),
        .bufferSize = static_cast < uint32_t>(sizeof(Vertex) * vertices.size())
    };

    m_vertexBuffer = std::make_unique<LocalBuffer>(m_device, desc);
}

void MainRenderPipeline::DestroyVertexBuffer() {
    m_vertexBuffer->Destroy();
    m_vertexBuffer = nullptr;
}

void MainRenderPipeline::CreateIndexBuffer() {
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    LocalBuffer::Desc desc = {
        .graphicsQueue = m_graphicsQueue,
        .transferQueue = m_transferQueue,
        .transferCommandBuffer = m_transferCommandBuffer,
        .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .buffer = indices.data(),
        .bufferSize = static_cast <uint32_t>(sizeof(uint16_t) * indices.size())
    };

    m_indexBuffer = std::make_unique<LocalBuffer>(m_device, desc);
}

void MainRenderPipeline::DestroyIndexBuffer() {
    m_indexBuffer->Destroy();
    m_indexBuffer = nullptr;
}

