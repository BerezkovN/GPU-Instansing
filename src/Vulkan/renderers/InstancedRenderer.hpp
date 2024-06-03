#pragma once

#include "MainRenderer.hpp"
#include "MainComponentSystem.hpp"

class InstancedRenderer : public MainRenderer
{
public:
	InstancedRenderer(const Context* context, MainComponentSystem* componentSystem);

	void Initialize(const std::string& vertexShader, const std::string& fragmentShader) override;
	void Destroy() override;

	void CreateInstanceBuffer();
	void UpdateInstanceBuffer();
	void DestroyInstanceBuffer();

	void Draw(VkCommandBuffer commandBuffer) override;
	void UpdateBuffers() override;

	MainRenderPipeline::VertexFormat GetVertexFormat() const override;

private:

	struct InstanceData
	{
		glm::vec4 translate;
		glm::vec4 rotation;
		MainComponentSystem::Sprite sprite;
	};

	std::unique_ptr<GenericBuffer> m_instancedBuffer;
};