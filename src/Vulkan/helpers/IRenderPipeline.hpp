#pragma once

#include <volk.h>

class IRenderPipeline
{
public:
	virtual ~IRenderPipeline() = default;

	virtual void Destroy() = 0;
	virtual void BeforeRender(VkCommandBuffer commandBuffer) = 0;

	virtual VkPipeline GetVkPipeline() const = 0;
};