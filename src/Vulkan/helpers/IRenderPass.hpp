#pragma once

#include <volk.h>
#include "Device.hpp"

class IRenderPass
{
public:
	virtual ~IRenderPass() = default;
	virtual void Destroy() = 0;
	virtual VkRenderPass GetVkRenderPass() const = 0;
};
