#pragma once

#include "../helpers/IRenderPipeline.hpp"

class Context;
class ShaderLayout;

class MainRenderPipeline : public IRenderPipeline
{
public:

	struct VertexFormat
	{
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;
	};
	MainRenderPipeline(const Context* context, const ShaderLayout* shaderLayout, const MainRenderPipeline::VertexFormat& desc);
	void Destroy() override;

	[[nodiscard]] VkPipeline GetVkPipeline() const override;

private:
	const Context* m_context;
	const ShaderLayout* m_shaderLayout;

	VkPipeline m_pipeline;

};
