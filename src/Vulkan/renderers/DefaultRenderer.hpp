#pragma once
#include "MainRenderer.hpp"

#include "MainComponentSystem.hpp"

class DefaultRenderer : public MainRenderer
{
public:
	DefaultRenderer(const Context* context, MainComponentSystem* componentSystem);

	void Initialize(const std::string& vertexShader, const std::string& fragmentShader) override;
	void Destroy() override;

	void Draw(VkCommandBuffer commandBuffer) override;
	
	MainRenderPipeline::VertexFormat GetVertexFormat() const override;

private:

	struct PerObject
	{
		glm::vec4 translate;
		MainComponentSystem::Sprite uv;
	};

	std::unique_ptr<GenericBuffer> m_perObjectUniformBuffer;
};