#pragma once

#include <volk.h>
#include <vector>


class IRenderer
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

	virtual ~IRenderer() = default;
	virtual void Destroy() = 0;

	virtual void RecordAndSubmit(const IRenderer::RecordDesc& desc) const = 0;
};