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
		std::optional<const DeviceQueue*> transferQueue;
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
	VkCommandPool m_graphicsCommandPool;
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers{};

	std::optional<const DeviceQueue*> m_transferQueue;
	std::optional<VkCommandPool> m_transferCommandPool;
	/**
	 * Could be from the graphics command pool
	 */
	VkCommandBuffer m_transferCommandBuffer;

	std::unique_ptr<GenericBuffer> m_vertexBuffer;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

};