#pragma once

#include <memory>
#include <glm/mat4x4.hpp>

#include "../helpers/IRenderer.hpp"

class Context;
class Shader;
class ShaderLayout;
class DeviceQueue;
class GenericBuffer;
class Sampler;
class IRenderPipeline;

class MainRenderer : public IRenderer
{
public:

	explicit MainRenderer(const Context* context);
	void Destroy() override;
	void Record(const MainRenderer::RecordDesc& desc) override;
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

	void CreateVertexBuffer();
	void DestroyVertexBuffer();

	void CreateIndexBuffer();
	void DestroyIndexBuffer();

	const Context* m_context;

	std::unique_ptr<Shader> m_fragmentShader;
	std::unique_ptr<Shader> m_vertexShader;
	std::unique_ptr<ShaderLayout> m_shaderLayout;

	std::unique_ptr<GenericBuffer> m_vertexBuffer;
	std::unique_ptr<GenericBuffer> m_indexBuffer;

	std::unique_ptr<GenericBuffer> m_uniformBuffer;

	std::unique_ptr<Sampler> m_sampler;

	std::unique_ptr<IRenderPipeline> m_mainRenderPipeline;
};
