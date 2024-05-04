#pragma once

#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/Device.hpp"

class MainRenderPipeline : public IRenderPipeline
{

public:

	MainRenderPipeline(const Device* device, const IRenderPass* renderPass, VkShaderModule fragmentShader, VkShaderModule vertexShader);
	void Destroy() override;
	void BeforeRender(VkCommandBuffer commandBuffer) override;

	VkPipeline GetVkPipeline() const override;

private:

	void CreateVertexBuffer();
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	const Device* m_device;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
};