#pragma once

#include "../helpers/IRenderer.hpp"
#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/Shader.hpp"
#include "../helpers/ShaderLayout.hpp"
#include "../helpers/Device.hpp"
#include "../helpers/buffers/GenericBuffer.hpp"

#include <glm/mat4x4.hpp>

class MainRenderer : public IRenderer
{
public:
	struct CreateDesc
	{
		const App* app;
		const Device* device;
		const IRenderPass* renderPass;
		const DeviceQueue* graphicsQueue;
		std::optional<const DeviceQueue*> transferQueue;
	};

	MainRenderer(const MainRenderer::CreateDesc& desc);
	void Destroy() override;
	void RecordAndSubmit(const MainRenderer::RecordDesc& desc) const override;
private:

	struct UniformBufferObject
	{
		// Beware of alignment!
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


	void CreateUniformBuffers();
	void UpdateUniformBuffers() const;
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

	const Device* m_device;
	const IRenderPass* m_renderPass;

	std::unique_ptr<Shader> m_fragmentShader;
	std::unique_ptr<Shader> m_vertexShader;
	std::unique_ptr<ShaderLayout> m_shaderLayout;

	const DeviceQueue* m_graphicsQueue;
	VkCommandPool m_graphicsCommandPool;
	VkCommandBuffer m_graphicsCommandBuffer;

	std::optional<const DeviceQueue*> m_transferQueue;
	std::optional<VkCommandPool> m_transferCommandPool;
	VkCommandBuffer m_transferCommandBuffer; // Could be from the graphics command pool

	std::unique_ptr<GenericBuffer> m_vertexBuffer;
	std::unique_ptr<GenericBuffer> m_indexBuffer;

	std::unique_ptr<GenericBuffer> m_uniformBuffer;

	std::unique_ptr<IRenderPipeline> m_mainRenderPipeline;
};
