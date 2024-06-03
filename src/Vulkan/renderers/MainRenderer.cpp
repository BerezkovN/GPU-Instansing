#include "MainRenderer.hpp"

#include <imgui.h>

#include "MainComponentSystem.hpp"
#include "MainRenderPipeline.hpp"

#include "../pch.hpp"
#include "../App.hpp"
#include "../helpers/buffers/LocalBuffer.hpp"
#include "../helpers/buffers/StagingBuffer.hpp"
#include "../helpers/textures/Sampler.hpp"
#include "../helpers/Shader.hpp"
#include "../helpers/ShaderLayout.hpp"


MainRenderer::MainRenderer(const Context* context, MainComponentSystem* componentSystem) {

    m_context = context;
    m_componentSystem = componentSystem;
}

void MainRenderer::Initialize(const std::string& vertexShader, const std::string& fragmentShader) {

    this->CreateVertexBuffer();
    this->CreateIndexBuffer();
    this->CreateUniformBuffers();

    m_vertexShader = std::make_unique<Shader>(m_context->GetDevice(), vertexShader, Shader::Type::Vertex);
    m_fragmentShader = std::make_unique<Shader>(m_context->GetDevice(), fragmentShader, Shader::Type::Fragment);

    m_shaderLayout = std::make_unique<ShaderLayout>(m_context->GetDevice(), m_vertexShader.get(), m_fragmentShader.get());

    m_mainRenderPipeline = std::make_unique<MainRenderPipeline>(m_context, m_shaderLayout.get(), this->GetVertexFormat());

    m_sampler = std::make_unique<Sampler>(m_context, "textures/Coin-sheet.png");

    m_shaderLayout->AttachBuffer("Matrices", m_uniformMatrixBuffer.get(), 0, m_uniformMatrixBuffer->GetBufferSize());
    m_shaderLayout->AttackSampler("DiffuseSampler", m_sampler.get());
}

void MainRenderer::Destroy() {

    m_sampler->Destroy();

    m_mainRenderPipeline->Destroy();
	m_vertexShader->Destroy();
    m_fragmentShader->Destroy();
    m_shaderLayout->Destroy();

    this->DestroyUniformBuffers();
    this->DestroyIndexBuffer();
    this->DestroyVertexBuffer();
}

void MainRenderer::Record(const MainRenderer::RecordDesc& desc) {

    const VkCommandBuffer commandBuffer = desc.commandBuffer;

    static bool updateBuffers = true;
    static int entityCount = static_cast<int>(m_componentSystem->GetEntityCount());

    ImGui::Checkbox("Update buffers", &updateBuffers);
    if (updateBuffers) {
        this->UpdateBuffers();
    }
    ImGui::SliderInt("Entity Count", &entityCount, 0, m_maxEntityCount);
    m_componentSystem->SetEntityCount(entityCount);

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

    m_shaderLayout->BindDescriptors(commandBuffer);

    this->Draw(commandBuffer);
}

void MainRenderer::UpdateBuffers() {

    this->UpdateUniformBuffers();
}

void MainRenderer::CreateUniformBuffers() {

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = VkBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(UniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    m_uniformMatrixBuffer = std::make_unique<GenericBuffer>(m_context, desc);
    m_uniformMatrixBuffer->MapMemory(m_uniformMatrixBuffer->GetBufferSize());

}

void MainRenderer::UpdateUniformBuffers() const {

    int width, height;
    m_context->GetScreenSize(width, height);

    static float camZOffset = -20;
    ImGui::SliderFloat("Camera Z Offset", &camZOffset, -200, 0);

    const MainRenderer::UniformBufferObject ubo = {
        .view = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 0, camZOffset)),
        .proj = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 1000.0f)
    };

    void* mappedUboPtr = m_uniformMatrixBuffer->GetMappedMemory();
    std::memcpy(mappedUboPtr, &ubo, sizeof(ubo));
}

void MainRenderer::DestroyUniformBuffers() {

    m_uniformMatrixBuffer->Destroy();
    m_uniformMatrixBuffer = nullptr;
}

void MainRenderer::CreateVertexBuffer() {
    const std::vector<MainRenderer::Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, -1},
        {{-0.5f,  0.5f, 0.0f}, 0xFF03102},
        {{ 0.5f,  0.5f, 0.0f}, -1},
        {{ 0.5f, -0.5f, 0.0f}, 0xFF03102},
    };

    LocalBuffer::Desc vertexDesc = {
        .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .buffer = vertices.data(),
        .bufferSize = sizeof(MainRenderer::Vertex) * vertices.size()
    };

    m_vertexBuffer = std::make_unique<LocalBuffer>(m_context, vertexDesc);
}

void MainRenderer::DestroyVertexBuffer() {

    m_vertexBuffer->Destroy();
    m_vertexBuffer = nullptr;
}

void MainRenderer::CreateIndexBuffer() {
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    LocalBuffer::Desc desc = {
        .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .buffer = indices.data(),
        .bufferSize = static_cast <uint32_t>(sizeof(uint16_t) * indices.size())
    };

    m_indexBuffer = std::make_unique<LocalBuffer>(m_context, desc);
}

void MainRenderer::DestroyIndexBuffer() {
    m_indexBuffer->Destroy();
    m_indexBuffer = nullptr;
}
