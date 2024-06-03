#include "InstancedRenderer.hpp"

#include "../pch.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

#include <tracy/Tracy.hpp>

#include "MainComponentSystem.hpp"

constexpr std::size_t constexpr_strlen(const char* str) {
    return *str ? 1 + constexpr_strlen(str + 1) : 0;
}

InstancedRenderer::InstancedRenderer(const Context* context, MainComponentSystem* componentSystem)
	: MainRenderer(context, componentSystem) {}

void InstancedRenderer::Initialize(const std::string& vertexShader, const std::string& fragmentShader) {
	MainRenderer::Initialize(vertexShader, fragmentShader);
    this->CreateInstanceBuffer();
}

void InstancedRenderer::Destroy() {
	MainRenderer::Destroy();
    this->DestroyInstanceBuffer();
}


void InstancedRenderer::CreateInstanceBuffer() {

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = VkBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = MainComponentSystem::kMaxEntityCount * sizeof(InstanceData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    m_instancedBuffer = std::make_unique<GenericBuffer>(m_context, desc);
    m_instancedBuffer->MapMemory(m_instancedBuffer->GetBufferSize());
}



void InstancedRenderer::UpdateInstanceBuffer() {

    ZoneScoped;

    const uint32_t instanceCount = m_componentSystem->GetEntityCount();

    static bool packData = true;
    ImGui::Checkbox("Pack data", &packData);

    static bool writeData = true;
    ImGui::Checkbox("Write data", &writeData);


	InstanceData data{};
    for (size_t ind = 0; ind < instanceCount; ind++) {

        if (packData) {
	        data.translate = m_componentSystem->GetTransforms()[ind].translate;
	        //data.rotation = {};
	        data.sprite = m_componentSystem->GetSprites()[ind];
        }

        if (writeData) {
	        const auto instances = static_cast<InstanceData*>(m_instancedBuffer->GetMappedMemory());
	        instances[ind] = data;
        }
    }
}

void InstancedRenderer::DestroyInstanceBuffer() {

    m_instancedBuffer->Destroy();
    m_instancedBuffer = nullptr;
}

void InstancedRenderer::Draw(VkCommandBuffer commandBuffer) {

    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer(), m_instancedBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0, 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, 6, m_componentSystem->GetEntityCount(), 0, 0, 0);
}


void InstancedRenderer::UpdateBuffers() {

	MainRenderer::UpdateBuffers();
    this->UpdateInstanceBuffer();
}


MainRenderPipeline::VertexFormat InstancedRenderer::GetVertexFormat() const {
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
	            .offset = offsetof(InstanceData, sprite)
	        }
	    }
    };
}

