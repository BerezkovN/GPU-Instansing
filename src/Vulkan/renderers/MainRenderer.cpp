#include "MainRenderer.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include "MainRenderPipeline.hpp"

#include "../pch.hpp"
#include "../App.hpp"
#include "../helpers/buffers/LocalBuffer.hpp"
#include "../helpers/textures/Sampler.hpp"
#include "../helpers/Shader.hpp"
#include "../helpers/ShaderLayout.hpp"
#include "../helpers/IRenderPass.hpp"


MainRenderer::MainRenderer(const Context* context) {

    m_context = context;

    this->CreateVertexBuffer();
    this->CreateIndexBuffer();
    this->CreateUniformBuffers();
    this->CreateInstances();

    m_vertexShader = std::make_unique<Shader>(m_context->GetDevice(), "shaders/triangle.vert.spv", Shader::Type::Vertex);
    m_fragmentShader = std::make_unique<Shader>(m_context->GetDevice(), "shaders/triangle.frag.spv", Shader::Type::Fragment);

    m_shaderLayout = std::make_unique<ShaderLayout>(m_context->GetDevice(), m_vertexShader.get(), m_fragmentShader.get());

    m_mainRenderPipeline = std::make_unique<MainRenderPipeline>(m_context, m_shaderLayout.get(), this->GetVertexFormat());

    m_sampler = std::make_unique<Sampler>(m_context, "textures/Tree.png");

    m_shaderLayout->AttachBuffer("UniformBufferObject", m_uniformBuffer.get(), 0, m_uniformBuffer->GetBufferSize());
    m_shaderLayout->AttackSampler("texSampler", m_sampler.get());
}

void MainRenderer::Destroy() {

    m_sampler->Destroy();

    m_mainRenderPipeline->Destroy();
	m_vertexShader->Destroy();
    m_fragmentShader->Destroy();
    m_shaderLayout->Destroy();

    this->DestroyInstances();
    this->DestroyUniformBuffers();
    this->DestroyIndexBuffer();
    this->DestroyVertexBuffer();
}

void MainRenderer::Record(const MainRenderer::RecordDesc& desc) {

    const VkCommandBuffer commandBuffer = desc.commandBuffer;

    static float clearColor[4] { 0.2f, 0.25f, 0.45f, 1.0f };

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool open = true;
	ImGui::Begin("Debug", &open);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::ColorEdit3("Clear color", clearColor);
    const VkClearValue clearValue = {
        .color = {.float32 = {clearColor[0], clearColor[1], clearColor[2], clearColor[3]}}
    };

    this->UpdateUniformBuffers();
    this->UpdateInstances();


    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = desc.renderPass->GetVkRenderPass();
    info.framebuffer = desc.framebuffer;
    info.renderArea = desc.renderArea;
    info.clearValueCount = 1;
    info.pClearValues = &clearValue;
    vkCmdBeginRenderPass(desc.commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

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


    // Learn about vkCmdBindVertexBuffers
    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer(), m_instancedBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0, 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

    m_shaderLayout->BindDescriptors(commandBuffer);

    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(m_instances.size()), 0, 0, 0);

	ImGui::End();
    ImGui::Render();
    ImDrawData* imGuiDrawData = ImGui::GetDrawData();
    const bool isMinimized = (imGuiDrawData->DisplaySize.x <= 0.0f || imGuiDrawData->DisplaySize.y <= 0.0f);
    if (!isMinimized)
    {
        ImGui_ImplVulkan_RenderDrawData(imGuiDrawData, commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void MainRenderer::CreateUniformBuffers() {

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = VkBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(UniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    m_uniformBuffer = std::make_unique<GenericBuffer>(m_context, desc);
    m_uniformBuffer->MapMemory(m_uniformBuffer->GetBufferSize());

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

    void* mappedUboPtr = m_uniformBuffer->GetMappedMemory();
    std::memcpy(mappedUboPtr, &ubo, sizeof(ubo));
}

void MainRenderer::DestroyUniformBuffers() {

    m_uniformBuffer->Destroy();
    m_uniformBuffer = nullptr;
}

void MainRenderer::CreateInstances() {
    std::random_device rndDevice;
    std::mt19937 rndEngine(rndDevice());
    std::uniform_real_distribution<> offsetDist(-100, 100);
    std::uniform_real_distribution<> zDist(-1, 1);

    for (int ind = 0; ind < 100000; ind++) {
        m_instances.emplace_back(InstanceData{ .translate = {offsetDist(rndEngine), offsetDist(rndEngine), zDist(rndEngine), 0}});
    }

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = VkBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = m_instances.size() * sizeof(InstanceData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    m_instancedBuffer = std::make_unique<GenericBuffer>(m_context, desc);
    m_instancedBuffer->MapMemory(m_instancedBuffer->GetBufferSize());
}

void MainRenderer::UpdateInstances() {

    const auto currentTime = std::chrono::high_resolution_clock::now();

    for (size_t ind = 0; ind < m_instances.size(); ind++) {

        auto& instance = m_instances[ind];
        instance.rotation.z += (ind % 2 ? -1 : 1) * ImGui::GetIO().DeltaTime;
    }

    std::memcpy(m_instancedBuffer->GetMappedMemory(), m_instances.data(), m_instances.size() * sizeof(InstanceData));
}

void MainRenderer::DestroyInstances() {

    m_instancedBuffer->Destroy();
    m_instancedBuffer = nullptr;
}

void MainRenderer::CreateVertexBuffer() {
    const std::vector<MainRenderer::Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, -1, {0.0, 0.0}},
        {{-0.5f,  0.5f, 0.0f}, 0xFF03102, {0.0, 1.0}},
        {{ 0.5f,  0.5f, 0.0f}, -1, {1.0, 1.0}},
        {{ 0.5f, -0.5f, 0.0f}, 0xFF03102, {1.0, 0.0}},
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

MainRenderPipeline::VertexFormat MainRenderer::GetVertexFormat() const {
    return {
        .bindings = {
            VkVertexInputBindingDescription {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            VkVertexInputBindingDescription {
                .binding = 1,
                .stride = sizeof(InstanceData),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            }
        },
        .attributes = {
            VkVertexInputAttributeDescription{
		        .location = 0,
		        .binding = 0,
		        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		        .offset = offsetof(Vertex, position)
		    },
		    VkVertexInputAttributeDescription{
		        .location = 1,
		        .binding = 0,
		        .format = VK_FORMAT_R8G8B8A8_UNORM,
		        .offset = offsetof(Vertex, color)
		    },
		    VkVertexInputAttributeDescription{
		        .location = 2,
		        .binding = 0,
		        .format = VK_FORMAT_R32G32_SFLOAT,
		        .offset = offsetof(Vertex, uv)
		    },
		    VkVertexInputAttributeDescription{
		        .location = 3,
		        .binding = 1,
		        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		        .offset = offsetof(InstanceData, translate)
		    },
		    VkVertexInputAttributeDescription{
		        .location = 4,
		        .binding = 1,
		        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		        .offset = offsetof(InstanceData, rotation)
		    }
        }
    };
}
