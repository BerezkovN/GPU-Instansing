#pragma once

#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/Device.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

class MainRenderPipeline : public IRenderPipeline
{

public:
	struct CreateDesc
	{
		uint32_t framesInFlight;
		const Device* device;
		const IRenderPass* renderPass;
		const DeviceQueue* graphicsQueue;
		const DeviceQueue* transferQueue;
		VkShaderModule fragmentShader;
		VkShaderModule vertexShader;
	};

	MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc);
	void Destroy() override;

	void RecordAndSubmit(const MainRenderPipeline::RecordDesc& desc) const override;

	VkPipeline GetVkPipeline() const override;

private:

	void CreateCommandPools();
	void DestroyCommandPools();

	void CreateCommandBuffers();
	void DestroyCommandBuffers();

	void CreateVertexBuffer();
	void DestroyVertexBuffer();

	uint32_t m_framesInFlight;
	const Device* m_device;
	const IRenderPass* m_renderPass;
	const DeviceQueue* m_graphicsQueue;
	const DeviceQueue* m_transferQueue;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_transferCommandPool;

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers{};
	VkCommandBuffer m_transferCommandBuffer;

	std::unique_ptr<GenericBuffer> m_vertexBuffer;

};