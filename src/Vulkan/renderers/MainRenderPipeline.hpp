#pragma once

#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/Device.hpp"

class MainRenderPipeline : public IRenderPipeline
{

public:

	MainRenderPipeline(const Device* device, const IRenderPass* renderPass, VkShaderModule fragmentShader, VkShaderModule vertexShader);
	void Destroy() override;
	VkPipeline GetVkPipeline() const override;

private:

	const Device* m_device;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
};