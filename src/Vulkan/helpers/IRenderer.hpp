#pragma once

#include <volk.h>

#include "IRenderPass.hpp"

class IRenderer
{
public:
	struct RecordDesc
	{
		VkRect2D renderArea;

		VkCommandBuffer commandBuffer;
		const IRenderPass* renderPass;
		VkFramebuffer framebuffer;
	};

	virtual ~IRenderer() = default;
	virtual void Destroy() = 0;

	virtual void Record(const IRenderer::RecordDesc& desc) = 0;
};