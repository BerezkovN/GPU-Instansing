#pragma once

#include <string>
#include <volk.h>

class Context;

class Sampler
{
public:


	Sampler(const Context* context, const std::string& path);
	void Destroy();

	[[nodiscard]] VkSampler GetVkSampler() const;
	[[nodiscard]] VkImageView GetVkImageView() const;
	[[nodiscard]] VkImageLayout GetVkImageLayout() const;

private:

	void LoadImage(const std::string& path);

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

	const Context* m_context;

	VkImage m_image{};
	VkDeviceMemory m_imageMemory{};

	VkImageView m_imageView{};
	VkSampler m_sampler{};

	VkImageLayout m_layout;

	VkDeviceSize m_allocatedMemorySize{};
};