#pragma once

#include "../helpers/IRenderer.hpp"
#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/ShaderManager.hpp"
#include "../helpers/Device.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

class MainRenderer : public IRenderer
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
	};

	MainRenderer(const MainRenderer::CreateDesc& desc);
	void Destroy() override;
	void RecordAndSubmit(const MainRenderer::RecordDesc& desc) const override;
private:


	void CreateUniformBuffers();
	void UpdateUniformBuffers(uint32_t currentImage) const;
	void DestroyUniformBuffers();

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

	std::unique_ptr<ShaderManager> m_shaderManager;

	const DeviceQueue* m_graphicsQueue;
	VkCommandPool m_graphicsCommandPool;
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers{};

	std::optional<const DeviceQueue*> m_transferQueue;
	std::optional<VkCommandPool> m_transferCommandPool;
	VkCommandBuffer m_transferCommandBuffer; // Could be from the graphics command pool

	std::unique_ptr<GenericBuffer> m_vertexBuffer;
	std::unique_ptr<GenericBuffer> m_indexBuffer;

	std::vector<std::unique_ptr<GenericBuffer>> m_uniformBuffers;

	std::unique_ptr<IRenderPipeline> m_mainRenderPipeline;
};