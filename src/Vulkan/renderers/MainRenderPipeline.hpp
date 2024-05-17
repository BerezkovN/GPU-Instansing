#pragma once

#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/Device.hpp"
#include "../helpers/ShaderLayout.hpp"

#include <glm/vec3.hpp>

class MainRenderPipeline : public IRenderPipeline
{
public:

	struct Vertex
	{
		glm::vec3 position;
		int color;
	};

	struct CreateDesc
	{
		const App* app;
		const Device* device;
		const IRenderPass* renderPass;
		const ShaderLayout* shaderLayout;
	};

	MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc);
	void Destroy() override;

	[[nodiscard]] VkPipeline GetVkPipeline() const override;

private:

	std::vector<VkVertexInputAttributeDescription> GetAttributes();

	const App* m_app;
	const Device* m_device;
	const IRenderPass* m_renderPass;
	const ShaderLayout* m_shaderLayout;

	VkPipeline m_pipeline;

};
