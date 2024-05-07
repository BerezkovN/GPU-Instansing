#pragma once

#include <volk.h>
#include <vector>

class IRenderPipeline
{
public:
	virtual ~IRenderPipeline() = default;

	virtual void Destroy() = 0;

	virtual void BindDescriptors(VkCommandBuffer buffer, uint32_t frameIndex) = 0;

	virtual VkPipeline GetVkPipeline() const = 0;
	virtual VkPipelineLayout GetVkPipelineLayout() const = 0;
};