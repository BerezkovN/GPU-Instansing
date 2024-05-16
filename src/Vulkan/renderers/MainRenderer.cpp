#include "MainRenderer.hpp"

#include "MainRenderPipeline.hpp"
#include "../pch.hpp"
#include "../App.hpp"
#include "../helpers/buffers/LocalBuffer.hpp"


MainRenderer::MainRenderer(const MainRenderer::CreateDesc& desc) {

    m_app = desc.app;
    m_device = desc.device;
    m_renderPass = desc.renderPass;
    m_graphicsQueue = desc.graphicsQueue;
    m_transferQueue = desc.transferQueue;

    this->CreateCommandPools();
    this->CreateCommandBuffers();
    this->CreateVertexBuffer();
    this->CreateIndexBuffer();
    this->CreateUniformBuffers();

    m_shaderManager = std::make_unique<ShaderManager>(m_device);
    const VkShaderModule fragModule = m_shaderManager->LoadShader("shaders/triangle.frag.spv");
    const VkShaderModule vertModule = m_shaderManager->LoadShader("shaders/triangle.vert.spv");

    std::vector<MainRenderPipeline::PipelineDescriptorSets> descriptorSets;


    MainRenderPipeline::PipelineDescriptorSetInfo uniformBufferObject = {
        .buffer = m_uniformBuffer->GetVkBuffer(),
        .offset = 0,
        .range = m_uniformBuffer->GetBufferSize(),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    };

    descriptorSets.push_back({
        .uniformBufferObject = uniformBufferObject
    });


    MainRenderPipeline::CreateDesc pipelineDesc = {
        .app = desc.app,
        .device = desc.device,
        .renderPass = desc.renderPass,
        .fragmentShader = fragModule,
        .vertexShader = vertModule,
        .descriptorSetWrites = descriptorSets
    };
    m_mainRenderPipeline = std::make_unique<MainRenderPipeline>(pipelineDesc);

}

void MainRenderer::Destroy() {

    m_mainRenderPipeline->Destroy();
    m_shaderManager->DestroyAllShaders();

    this->DestroyUniformBuffers();
    this->DestroyIndexBuffer();
    this->DestroyVertexBuffer();
    this->DestroyCommandBuffers();
    this->DestroyCommandPools();
}

void MainRenderer::RecordAndSubmit(const MainRenderer::RecordDesc& desc) const {

    VkCommandBuffer commandBuffer = m_graphicsCommandBuffer;

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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_mainRenderPipeline->GetVkPipeline());

    this->UpdateUniformBuffers();

    // Learn about vkCmdBindVertexBuffers
    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

    m_mainRenderPipeline->BindDescriptors(commandBuffer);

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


void MainRenderer::CreateUniformBuffers() {

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(MainRenderPipeline::UniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    m_uniformBuffer = std::make_unique<GenericBuffer>(m_device, desc);
    m_uniformBuffer->MapMemory(m_uniformBuffer->GetBufferSize());

}

void MainRenderer::UpdateUniformBuffers() const {

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    int width, height;
    m_app->GetScreenSize(width, height);

    MainRenderPipeline::UniformBufferObject ubo = {
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 10.0f)
    };

    ubo.proj[1][1] *= -1;

    // TODO: Learn about push constants
    void* mappedUboPtr = m_uniformBuffer->GetMappedMemory();
    std::memcpy(mappedUboPtr, &ubo, sizeof(ubo));
}

void MainRenderer::DestroyUniformBuffers() {

    m_uniformBuffer->Destroy();
    m_uniformBuffer = nullptr;
}



void MainRenderer::CreateCommandPools() {
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

void MainRenderer::DestroyCommandPools() {

    if (m_transferCommandPool.has_value()) {
        vkDestroyCommandPool(m_device->GetVkDevice(), m_transferCommandPool.value(), nullptr);
    }
    vkDestroyCommandPool(m_device->GetVkDevice(), m_graphicsCommandPool, nullptr);

    m_transferCommandPool = VK_NULL_HANDLE;
    m_graphicsCommandPool = VK_NULL_HANDLE;
}

void MainRenderer::CreateCommandBuffers() {
    
    const VkCommandBufferAllocateInfo graphicsBuffersInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(m_device->GetVkDevice(), &graphicsBuffersInfo, &m_graphicsCommandBuffer);
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

void MainRenderer::DestroyCommandBuffers() {

    vkFreeCommandBuffers(m_device->GetVkDevice(),
        m_transferCommandPool.has_value() ? m_transferCommandPool.value() : m_graphicsCommandPool,
        1, &m_transferCommandBuffer);

    vkFreeCommandBuffers(m_device->GetVkDevice(), m_graphicsCommandPool, 1, &m_graphicsCommandBuffer);

    m_graphicsCommandBuffer = VK_NULL_HANDLE;
    m_transferCommandBuffer = VK_NULL_HANDLE;

}

void MainRenderer::CreateVertexBuffer() {
    const std::vector<MainRenderPipeline::Vertex> vertices = {
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
        .bufferSize = static_cast <uint32_t>(sizeof(MainRenderPipeline::Vertex) * vertices.size())
    };

    m_vertexBuffer = std::make_unique<LocalBuffer>(m_device, desc);
}

void MainRenderer::DestroyVertexBuffer() {
    m_vertexBuffer->Destroy();
    m_vertexBuffer = nullptr;
}

void MainRenderer::CreateIndexBuffer() {
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

void MainRenderer::DestroyIndexBuffer() {
    m_indexBuffer->Destroy();
    m_indexBuffer = nullptr;
}