#include "InstancedCachedRenderer.hpp"

#include "../pch.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"
#include "../helpers/Context.hpp"


InstancedCachedRenderer::InstancedCachedRenderer(const Context* context) : MainRenderer(context) {}

void InstancedCachedRenderer::CreateInstances() {


    std::random_device rndDevice;
    std::mt19937 rndEngine(rndDevice());

    std::uniform_real_distribution<> offsetDist(-100, 100);
    std::uniform_real_distribution<> zDist(-1, 1);
    std::uniform_int_distribution<> animationDist(0, 2);

    m_instanceMoveComponents.resize(kMaxInstanceCount);
    m_instanceTransforms.reserve(kMaxInstanceCount);
    m_instanceSprites.reserve(kMaxInstanceCount);
    m_instanceAnimations.reserve(kMaxInstanceCount);

    for (uint32_t ind = 0; ind < kMaxInstanceCount; ind++) {
        m_instanceTransforms.push_back(Transform{
            .translate = {
                offsetDist(rndEngine), offsetDist(rndEngine), zDist(rndEngine), 0
            },
            .rotation = {}
        });

        m_instanceSprites.push_back(Sprite{
            .topLeft = {0, 0},
            .bottomRight = {1 / 8.0f, 1.0f}
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
            .size = kMaxInstanceCount * sizeof(InstanceData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
    };

    m_instancedBuffer = std::make_unique<GenericBuffer>(m_context, desc);
    m_instancedBuffer->MapMemory(m_instancedBuffer->GetBufferSize());
}

void InstancedCachedRenderer::UpdateInstances(uint32_t instanceCount) {

    for (size_t ind = 0; ind < instanceCount; ind++) {

        m_instanceMoveComponents[ind].offset = glm::vec4(0, sin(ind + glfwGetTime()), 0, 0);
        m_instanceAnimations[ind].currentFrame = static_cast<uint32_t>((static_cast<float>(glfwGetTime()) / m_instanceAnimations[ind].delay) * static_cast<float>(m_instanceAnimations[ind].frameCount)) + ind;
        const float animation = static_cast<float> (m_instanceAnimations[ind].currentFrame) / static_cast<float>(m_instanceAnimations[ind].frameCount);

        InstanceData* instances = static_cast<InstanceData*>(m_instancedBuffer->GetMappedMemory());
        auto& instance = instances[ind];

        instance.translate = m_instanceTransforms[ind].translate + m_instanceMoveComponents[ind].offset;
        instance.rotation = m_instanceTransforms[ind].rotation;
        instance.uv = glm::vec4(m_instanceSprites[ind].topLeft.x + animation, m_instanceSprites[ind].bottomRight.x + animation, m_instanceSprites[ind].topLeft.y, m_instanceSprites[ind].bottomRight.y);
    }
    
    const VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = m_instancedBuffer->GetVkDeviceMemory(),
        .offset = 0,
        .size = instanceCount * sizeof(InstanceData)
    };
    const VkResult result = vkFlushMappedMemoryRanges(m_context->GetDevice()->GetVkDevice(), 1, &range);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[InstancedCachedRenderer] Could not flush the memory");
    }
}


MainRenderPipeline::VertexFormat InstancedCachedRenderer::GetVertexFormat() const {
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

