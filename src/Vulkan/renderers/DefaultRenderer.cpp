#include "DefaultRenderer.hpp"

#include "MainComponentSystem.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

DefaultRenderer::DefaultRenderer(const Context* context, MainComponentSystem* componentSystem)
	: MainRenderer(context, componentSystem) {

	componentSystem->SetEntityCount(100);
}

void DefaultRenderer::Initialize(const std::string& vertexShader, const std::string& fragmentShader) {
	MainRenderer::Initialize(vertexShader, fragmentShader);

	GenericBuffer::Desc desc = {
	.bufferCreateInfo = VkBufferCreateInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(PerObject),
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		},
		.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};

	m_perObjectUniformBuffer = std::make_unique<GenericBuffer>(m_context, desc);
	m_perObjectUniformBuffer->MapMemory(m_perObjectUniformBuffer->GetBufferSize());

	m_shaderLayout->AttachBuffer("PerObject", m_perObjectUniformBuffer.get(), 0, m_perObjectUniformBuffer->GetBufferSize());
}

void DefaultRenderer::Destroy() {
	MainRenderer::Destroy();

	m_perObjectUniformBuffer->Destroy();
	m_perObjectUniformBuffer = nullptr;
}

void DefaultRenderer::Draw(VkCommandBuffer commandBuffer) {

	const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer() };
	constexpr VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t ind = 0; ind < m_componentSystem->GetEntityCount(); ind++) {

		const auto perObject = static_cast<PerObject*>(m_perObjectUniformBuffer->GetMappedMemory());
		perObject->translate = m_componentSystem->GetTransforms()[ind].translate;
		perObject->uv = m_componentSystem->GetSprites()[ind];

		vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
	}

}

MainRenderPipeline::VertexFormat DefaultRenderer::GetVertexFormat() const {
	return {
		.bindings = {
			VkVertexInputBindingDescription {
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			}
		},
		.attributes = 
		{

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
			}
		}
	};
}

