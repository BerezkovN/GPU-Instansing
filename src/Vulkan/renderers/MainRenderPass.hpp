#pragma once

#include "../helpers/IRenderPass.hpp"

class MainRenderPass : public IRenderPass
{
public:
	void Initialize(const Device* device, const Swapchain* swapchain) override;

	void Destroy() override;
	[[nodiscard]] VkRenderPass GetVkRenderPass() const override;

private:
	const Device* m_device{};
	VkRenderPass m_renderPass{};
};