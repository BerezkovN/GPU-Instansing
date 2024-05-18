#pragma once

#include <volk.h>

class Device;
class Swapchain;

class IRenderPass
{
public:
	virtual ~IRenderPass() = default;
	
	virtual void Initialize(const Device* device, const Swapchain* swapchain) = 0;
	virtual void Destroy() = 0;
	[[nodiscard]] virtual VkRenderPass GetVkRenderPass() const = 0;
};
