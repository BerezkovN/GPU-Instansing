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

    m_sampler = std::make_unique<Sampler>(m_context, "textures/Coin-sheet.png");

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

    static bool updateInstances = true;
    ImGui::Checkbox("Update instances", &updateInstances);
    if (updateInstances) {
		this->UpdateInstances();
    }

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


    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer(), m_instancedBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0, 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

    m_shaderLayout->BindDescriptors(commandBuffer);

    static int instanceCount = static_cast<int>(m_instanceTransforms.size());
    ImGui::SliderInt("Instance Count", &instanceCount, 0, static_cast<int>(m_instanceTransforms.size()));
    vkCmdDrawIndexed(commandBuffer, 6, instanceCount, 0, 0, 0);

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

	constexpr uint32_t instanceCount = 100000;
    
	std::random_device rndDevice;
    std::mt19937 rndEngine(rndDevice());

    std::uniform_real_distribution<> offsetDist(-100, 100);
    std::uniform_real_distribution<> zDist(-1, 1);
    std::uniform_int_distribution<> animationDist(0, 2);

    m_instances.resize(instanceCount);
    m_instanceMoveComponents.resize(instanceCount);

    m_instanceTransforms.reserve(instanceCount);
    m_instanceSprites.reserve(instanceCount);
    m_instanceAnimations.reserve(instanceCount);

    for (uint32_t ind = 0; ind < instanceCount; ind++) {
        m_instanceTransforms.push_back(Transform{
            .translate = {
                offsetDist(rndEngine), offsetDist(rndEngine), zDist(rndEngine), 0
            },
            .rotation = {}
        });

        m_instanceSprites.push_back(Sprite{
            .topLeft = {0, 0},
            .bottomRight = {1/8.0f, 1.0f}
        });

        m_instanceAnimations.push_back(Animation{
            .currentFrame = static_cast<uint32_t>(animationDist(rndEngine)),
            .frameCount = 8,
            .delay = 0.6f
        });
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

    for (size_t ind = 0; ind < m_instances.size(); ind++) {

        m_instanceMoveComponents[ind].offset = glm::vec4(0, sin(ind + glfwGetTime()), 0, 0);
        m_instanceAnimations[ind].currentFrame = static_cast<uint32_t>((static_cast<float>(glfwGetTime()) / m_instanceAnimations[ind].delay) * static_cast<float>(m_instanceAnimations[ind].frameCount)) + ind;
        float animation = static_cast<float> (m_instanceAnimations[ind].currentFrame) / static_cast<float>(m_instanceAnimations[ind].frameCount);

        auto& instance = m_instances[ind];


        instance.translate = m_instanceTransforms[ind].translate + m_instanceMoveComponents[ind].offset;
        instance.rotation = m_instanceTransforms[ind].rotation;
        instance.uv = glm::vec4(m_instanceSprites[ind].topLeft.x + animation, m_instanceSprites[ind].bottomRight.x + animation, m_instanceSprites[ind].topLeft.y, m_instanceSprites[ind].bottomRight.y);
    }

    std::memcpy(m_instancedBuffer->GetMappedMemory(), m_instances.data(), m_instances.size() * sizeof(InstanceData));
}

void MainRenderer::DestroyInstances() {

    m_instancedBuffer->Destroy();
    m_instancedBuffer = nullptr;
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
            // Per Vertex
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

            // Per Instance
		    VkVertexInputAttributeDescription{
		        .location = 2,
		        .binding = 1,
		        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		        .offset = offsetof(InstanceData, translate)
		    },
		    VkVertexInputAttributeDescription{
		        .location = 3,
		        .binding = 1,
		        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		        .offset = offsetof(InstanceData, rotation)
		    },
		    VkVertexInputAttributeDescription{
		        .location = 4,
		        .binding = 1,
		        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		        .offset = offsetof(InstanceData, uv)
		    }
        }
    };
}
