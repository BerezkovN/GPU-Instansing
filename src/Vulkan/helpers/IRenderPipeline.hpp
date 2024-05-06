#pragma once

#include <volk.h>
#include <vector>

class IRenderPipeline
{
public:
	struct RecordDesc
	{
		uint32_t frameIndex;
		VkRect2D renderArea;
		VkFramebuffer framebuffer;

		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence submitFrameFence;
	};

	virtual ~IRenderPipeline() = default;

	virtual void Destroy() = 0;

	virtual void RecordAndSubmit(const IRenderPipeline::RecordDesc& desc) const = 0;
	virtual VkPipeline GetVkPipeline() const = 0;
};