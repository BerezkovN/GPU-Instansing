#pragma once

#include <memory>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "MainRenderPipeline.hpp"
#include "../helpers/IRenderer.hpp"
#include "../helpers/textures/Sampler.hpp"
#include "../helpers/ShaderLayout.hpp"
#include "../helpers/Shader.hpp"

class Context;
class DeviceQueue;
class GenericBuffer;
class StagingBuffer;
class IRenderPipeline;
class MainComponentSystem;

class MainRenderer : public IRenderer
{
public:

	explicit MainRenderer(const Context* context, MainComponentSystem* componentSystem);

	void Initialize(const std::string& vertexShader, const std::string& fragmentShader) override;
	void Destroy() override;

	void Record(const MainRenderer::RecordDesc& desc) override;

protected:

	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct Vertex
	{
		glm::vec3 position;
		int color;
	};

	virtual void BindBuffers(VkCommandBuffer commandBuffer);
	virtual void UpdateBuffers();

	void CreateUniformBuffers();
	void UpdateUniformBuffers() const;
	void DestroyUniformBuffers();

	void CreateVertexBuffer();
	void DestroyVertexBuffer();

	void CreateIndexBuffer();
	void DestroyIndexBuffer();

	virtual MainRenderPipeline::VertexFormat GetVertexFormat() const = 0;

	const Context* m_context;
	MainComponentSystem* m_componentSystem;

	std::unique_ptr<Shader> m_fragmentShader;
	std::unique_ptr<Shader> m_vertexShader;
	std::unique_ptr<ShaderLayout> m_shaderLayout;

	std::unique_ptr<GenericBuffer> m_vertexBuffer;
	std::unique_ptr<GenericBuffer> m_indexBuffer;
	std::unique_ptr<GenericBuffer> m_uniformBuffer;

	std::unique_ptr<Sampler> m_sampler;
	std::unique_ptr<IRenderPipeline> m_mainRenderPipeline;
};
