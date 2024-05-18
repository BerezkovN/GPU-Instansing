#pragma once

#include <string>

#include "../Device.hpp"

class Sampler
{
public:
	struct Desc
	{
		const DeviceQueue* graphicsQueue;
		std::optional<const DeviceQueue*> transferQueue;

		VkCommandBuffer transferCommandBuffer;
	};

	Sampler(const Device* device, const std::string& path, const Sampler::Desc& desc);
	void Destroy();

private:

	void LoadImage(const Sampler::Desc& desc, const std::string& path);

	struct TransitionDesc
	{
		VkImageLayout oldLayout;
		VkImageLayout newLayout;
		VkAccessFlags srcAccess;
		VkAccessFlags dstAccess;
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
	};
	void TransitionLayout(VkCommandBuffer commandBuffer, const TransitionDesc& desc) const;

	void AllocateImage(VkMemoryPropertyFlags memoryProperty);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	const Device* m_device{};

	VkImage m_image{};
	VkDeviceMemory m_imageMemory{};

	VkImageView m_imageView{};

	VkSampler m_sampler{};

	VkDeviceSize m_allocatedMemorySize{};
};