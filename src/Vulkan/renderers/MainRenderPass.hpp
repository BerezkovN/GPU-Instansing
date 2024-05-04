#pragma once

#include "../helpers/IRenderPass.hpp"
#include "../helpers/Device.hpp"
#include "../helpers/Swapchain.hpp"

class MainRenderPass : public IRenderPass
{
public:
	MainRenderPass(const Device* device, const Swapchain* swapchain);

	void Destroy() override;
	VkRenderPass GetVkRenderPass() const override;

private:
	const Device* m_device;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
};