#pragma once
#include "MainRenderer.hpp"

#include "MainComponentSystem.hpp"

class DefaultRenderer : public MainRenderer
{
public:
	DefaultRenderer(const Context* context, MainComponentSystem* componentSystem);

	void Draw(VkCommandBuffer commandBuffer) override;
	
	MainRenderPipeline::VertexFormat GetVertexFormat() const override;

private:

	struct PerObject
	{
		glm::vec4 translate;
		MainComponentSystem::Sprite uv;
	};

};