#pragma once

#include "MainRenderer.hpp"

class InstancedCachedRenderer : public MainRenderer
{
public:
	InstancedCachedRenderer(const Context* context);

	void CreateInstances() override;
	void UpdateInstances(uint32_t instanceCount) override;

	MainRenderPipeline::VertexFormat GetVertexFormat() const override;

private:

	struct InstanceData
	{
		glm::vec4 translate;
		glm::vec4 rotation;
		glm::vec4 uv;
	};
};