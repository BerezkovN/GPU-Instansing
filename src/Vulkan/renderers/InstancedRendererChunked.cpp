#include "InstancedRendererChunked.hpp"

#include "InstancedRenderer.hpp"

#include "../pch.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

#include <tracy/Tracy.hpp>

#include "MainComponentSystem.hpp"

constexpr std::size_t constexpr_strlen(const char* str) {
    return *str ? 1 + constexpr_strlen(str + 1) : 0;
}

InstancedRendererChunked::InstancedRendererChunked(const Context* context, MainComponentSystem* componentSystem)
    : MainRenderer(context, componentSystem) {}

void InstancedRendererChunked::Initialize(const std::string& vertexShader, const std::string& fragmentShader) {
    MainRenderer::Initialize(vertexShader, fragmentShader);
    this->CreateInstanceBuffers();
}

void InstancedRendererChunked::Destroy() {
    MainRenderer::Destroy();
    this->DestroyInstanceBuffers();
}


void InstancedRendererChunked::CreateInstanceBuffers() {


    m_instancedTranslationBuffer = std::make_unique<GenericBuffer>(m_context, GenericBuffer::Desc{
        .bufferCreateInfo = VkBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = MainComponentSystem::kMaxEntityCount * sizeof(glm::vec4),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    });
    m_instancedTranslationBuffer->MapMemory(m_instancedTranslationBuffer->GetBufferSize());

    m_instancedRotationBuffer = std::make_unique<GenericBuffer>(m_context, GenericBuffer::Desc{
	    .bufferCreateInfo = VkBufferCreateInfo {
	        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	        .size = MainComponentSystem::kMaxEntityCount * sizeof(glm::vec4),
	        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    },
	    .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    });
    m_instancedRotationBuffer->MapMemory(m_instancedRotationBuffer->GetBufferSize());

    m_instancedSpriteBuffer = std::make_unique<GenericBuffer>(m_context, GenericBuffer::Desc{
	    .bufferCreateInfo = VkBufferCreateInfo {
	        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	        .size = MainComponentSystem::kMaxEntityCount * sizeof(MainComponentSystem::Sprite),
	        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    },
	    .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    });
    m_instancedSpriteBuffer->MapMemory(m_instancedSpriteBuffer->GetBufferSize());
}



void InstancedRendererChunked::UpdateInstanceBuffers() {

    ZoneScoped;

    const uint32_t instanceCount = m_componentSystem->GetEntityCount();

    static bool writeData = true;
    ImGui::Checkbox("Write data", &writeData);

    if (!writeData) {
        return;
    }

    for (size_t ind = 0; ind < instanceCount; ind++) {
        const auto translationBuffer = static_cast<glm::vec4*>(m_instancedTranslationBuffer->GetMappedMemory());
        translationBuffer[ind] = m_componentSystem->GetTransforms()[ind].translate;
    }

    for (size_t ind = 0; ind < instanceCount; ind++) {
        const auto rotationBuffer = static_cast<glm::vec4*>(m_instancedRotationBuffer->GetMappedMemory());
        rotationBuffer[ind] = {};
    }

    for (size_t ind = 0; ind < instanceCount; ind++) {
        const auto spriteBuffer = static_cast<MainComponentSystem::Sprite*>(m_instancedSpriteBuffer->GetMappedMemory());
        spriteBuffer[ind] = m_componentSystem->GetSprites()[ind];
    }
}

void InstancedRendererChunked::DestroyInstanceBuffers() {

    m_instancedRotationBuffer->Destroy();
    m_instancedRotationBuffer = nullptr;
    m_instancedSpriteBuffer->Destroy();
    m_instancedSpriteBuffer = nullptr;
    m_instancedTranslationBuffer->Destroy();
    m_instancedTranslationBuffer = nullptr;
}

void InstancedRendererChunked::BindBuffers(VkCommandBuffer commandBuffer) {

    const VkBuffer vertexBuffers[] = {
    	m_vertexBuffer->GetVkBuffer(),
    	m_instancedTranslationBuffer->GetVkBuffer(),
    	m_instancedRotationBuffer->GetVkBuffer(),
    	m_instancedSpriteBuffer->GetVkBuffer(),
    };
    constexpr VkDeviceSize offsets[] = { 0, 0, 0, 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 4, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);
}


void InstancedRendererChunked::UpdateBuffers() {

    MainRenderer::UpdateBuffers();
    this->UpdateInstanceBuffers();
}


MainRenderPipeline::VertexFormat InstancedRendererChunked::GetVertexFormat() const {
    return {
        .bindings = {
            VkVertexInputBindingDescription {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            VkVertexInputBindingDescription {
                .binding = 1,
                .stride = sizeof(glm::vec4),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            },
            VkVertexInputBindingDescription {
                .binding = 2,
                .stride = sizeof(glm::vec4),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            },
            VkVertexInputBindingDescription {
                .binding = 3,
                .stride = sizeof(MainComponentSystem::Sprite),
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
	            .offset = 0
	        },
	        VkVertexInputAttributeDescription{
	            .location = 3,
	            .binding = 2,
	            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
	            .offset = 0
	        },
	        VkVertexInputAttributeDescription{
	            .location = 4,
	            .binding = 3,
	            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
	            .offset = 0
	        }
	    }
    };
}

