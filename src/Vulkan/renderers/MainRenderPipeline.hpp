#pragma once

#include "../helpers/IRenderPipeline.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Context;
class ShaderLayout;

class MainRenderPipeline : public IRenderPipeline
{
public:

	struct Vertex
	{
		glm::vec3 position;
		int color;
		glm::vec2 uv;
	};

	struct InstanceData
	{
		glm::vec4 translate;
	};

	MainRenderPipeline(const Context* context, const ShaderLayout* shaderLayout);
	void Destroy() override;

	[[nodiscard]] VkPipeline GetVkPipeline() const override;

private:

	std::vector<VkVertexInputAttributeDescription> GetAttributes();

	const Context* m_context;
	const ShaderLayout* m_shaderLayout;

	VkPipeline m_pipeline;

};
