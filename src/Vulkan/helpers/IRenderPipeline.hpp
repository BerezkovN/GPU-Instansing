#pragma once

#include <volk.h>
#include <vector>

class IRenderPipeline
{
public:
	virtual ~IRenderPipeline() = default;
	virtual void Destroy() = 0;

	[[nodiscard]] virtual VkPipeline GetVkPipeline() const = 0;
};