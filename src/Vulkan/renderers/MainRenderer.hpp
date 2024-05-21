#pragma once

#include <memory>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

class MainRenderer : public IRenderer
{
public:

	const uint32_t kMaxInstanceCount = 100000;

	explicit MainRenderer(const Context* context);
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

	struct Transform
	{
		glm::vec4 translate;
		glm::vec4 rotation;
	};

	struct Sprite
	{
		glm::vec2 topLeft;
		glm::vec2 bottomRight;
	};

	struct MoveComponent
	{
		glm::vec4 offset;
	};

	struct Animation
	{
		uint32_t currentFrame;
		uint32_t frameCount;
		float delay;
	};

	void CreateUniformBuffers();
	void UpdateUniformBuffers() const;
	void DestroyUniformBuffers();

	virtual void CreateInstances() = 0;
	virtual void UpdateInstances(uint32_t instanceCount) = 0;
	void DestroyInstances();

	void CreateVertexBuffer();
	void DestroyVertexBuffer();

	void CreateIndexBuffer();
	void DestroyIndexBuffer();

	virtual MainRenderPipeline::VertexFormat GetVertexFormat() const = 0;

	const Context* m_context;

	std::unique_ptr<Shader> m_fragmentShader;
	std::unique_ptr<Shader> m_vertexShader;
	std::unique_ptr<ShaderLayout> m_shaderLayout;

	std::unique_ptr<GenericBuffer> m_vertexBuffer;
	std::unique_ptr<GenericBuffer> m_indexBuffer;
	std::unique_ptr<GenericBuffer> m_uniformBuffer;
	std::unique_ptr<GenericBuffer> m_instancedBuffer;

	std::vector<Transform> m_instanceTransforms;
	std::vector<MoveComponent> m_instanceMoveComponents;

	std::vector<Sprite> m_instanceSprites;
	std::vector<Animation> m_instanceAnimations;

	std::unique_ptr<Sampler> m_sampler;
	std::unique_ptr<IRenderPipeline> m_mainRenderPipeline;
};
