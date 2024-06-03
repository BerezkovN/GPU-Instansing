#include "DefaultRenderer.hpp"

#include "MainComponentSystem.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

DefaultRenderer::DefaultRenderer(const Context* context, MainComponentSystem* componentSystem)
	: MainRenderer(context, componentSystem) {

	componentSystem->SetEntityCount(100);
	m_maxEntityCount = 20000;
}


void DefaultRenderer::Draw(VkCommandBuffer commandBuffer) {

	const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer() };
	constexpr VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t ind = 0; ind < m_componentSystem->GetEntityCount(); ind++) {

		PerObject perObject = {
			.translate = m_componentSystem->GetTransforms()[ind].translate,
			.uv = m_componentSystem->GetSprites()[ind]
		};

		vkCmdPushConstants(commandBuffer, m_shaderLayout->GetVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PerObject), &perObject);
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

