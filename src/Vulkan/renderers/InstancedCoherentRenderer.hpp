#pragma once

#include "MainRenderer.hpp"

class InstancedCoherentRenderer : public MainRenderer
{
public:
	InstancedCoherentRenderer(const Context* context);

	void CreateInstances() override;
	void UpdateInstances(uint32_t instanceCount) override;

	MainRenderPipeline::VertexFormat GetVertexFormat() const override;

private:

	struct InstanceData
	{
		glm::vec4 translate;
		glm::vec4 rotation;
		glm::vec4 uv;
		//glm::vec4 padding;
	};
};