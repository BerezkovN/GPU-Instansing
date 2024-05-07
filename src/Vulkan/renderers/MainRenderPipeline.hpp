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
		const App* app;
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

	void CreateDescriptorSetLayout();
	void DestroyDescriptorSetLayout();

	void CreateUniformBuffers();
	void UpdateUniformBuffers(uint32_t currentImage) const;
	void DestroyUniformBuffers();

	void CreateDescriptorPool();
	void DestroyDescriptorPool();

	void CreateDescriptorSets();

	void CreateCommandPools();
	void DestroyCommandPools();

	void CreateCommandBuffers();
	void DestroyCommandBuffers();

	void CreateVertexBuffer();
	void DestroyVertexBuffer();

	void CreateIndexBuffer();
	void DestroyIndexBuffer();

	const App* m_app;
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
	std::unique_ptr<GenericBuffer> m_indexBuffer;

	std::vector<std::unique_ptr<GenericBuffer>> m_uniformBuffers;

	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

};