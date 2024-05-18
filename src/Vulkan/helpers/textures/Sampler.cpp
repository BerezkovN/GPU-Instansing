#include "Sampler.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../../pch.hpp"
#include "../buffers/StagingBuffer.hpp"

Sampler::Sampler(const Device* device, const std::string& path, const Sampler::Desc& desc) {

	m_device = device;

	this->LoadImage(desc, path);

	const VkImageViewCreateInfo viewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = m_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	VkResult result = vkCreateImageView(m_device->GetVkDevice(), &viewCreateInfo, nullptr, &m_imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Sampler] Could not create image view");
	}

	constexpr VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = false,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	result = vkCreateSampler(m_device->GetVkDevice(), &samplerCreateInfo, nullptr, &m_sampler);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Sampler] Could not create sampler");
	}
}

void Sampler::Destroy() {

	vkDestroySampler(m_device->GetVkDevice(), m_sampler, nullptr);
	vkDestroyImageView(m_device->GetVkDevice(), m_imageView, nullptr);

	vkDestroyImage(m_device->GetVkDevice(), m_image, nullptr);
	vkFreeMemory(m_device->GetVkDevice(), m_imageMemory, nullptr);

	m_image = VK_NULL_HANDLE;
	m_imageMemory = VK_NULL_HANDLE;
}

void Sampler::LoadImage(const Sampler::Desc& desc, const std::string& path) {
	int width, height;
	int channelCount;

	stbi_uc* pixels = stbi_load(path.data(), &width, &height, &channelCount, STBI_rgb_alpha);
	const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

	if (!pixels) {
		throw std::runtime_error("[Sampler] Could not load sampler from path: " + path);
	}

	const StagingBuffer::Desc stagingBufferDesc = {
		.graphicsQueue = desc.graphicsQueue,
		.transferQueue = desc.transferQueue,
		.bufferSize = imageSize
	};
	StagingBuffer stagingBuffer(m_device, stagingBufferDesc);

	void* mappedMemory = stagingBuffer.MapMemory(stagingBuffer.GetBufferSize());
	std::memcpy(mappedMemory, pixels, stagingBuffer.GetBufferSize());
	stagingBuffer.UnmapMemory();

	stbi_image_free(pixels);

	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent = VkExtent3D {
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	const VkResult result = vkCreateImage(m_device->GetVkDevice(), &imageInfo, nullptr, &m_image);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Sampler] Could not create image: " + std::to_string(result));
	}

	this->AllocateImage(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,   // This means that data is tightly packed. 
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = VkOffset3D { 0, 0, 0 },
		.imageExtent = VkExtent3D { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
	};

	constexpr VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(desc.transferCommandBuffer, &beginInfo);

	this->TransitionLayout(desc.transferCommandBuffer, TransitionDesc{
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcAccess = VK_ACCESS_NONE, .dstAccess = VK_ACCESS_NONE,
		.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT
	});
	vkCmdCopyBufferToImage(desc.transferCommandBuffer, stagingBuffer.GetVkBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	this->TransitionLayout(desc.transferCommandBuffer, TransitionDesc{
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcAccess = VK_ACCESS_NONE, .dstAccess = VK_ACCESS_NONE,
		.srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
	});

	vkEndCommandBuffer(desc.transferCommandBuffer);

	stagingBuffer.Destroy();
}

void Sampler::TransitionLayout(VkCommandBuffer commandBuffer, const TransitionDesc& desc) const {

	const VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = desc.srcAccess,
		.dstAccessMask = desc.dstAccess,
		.oldLayout = desc.oldLayout,
		.newLayout = desc.newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vkCmdPipelineBarrier(commandBuffer,
		desc.srcStage, desc.dstStage,
		0,
		0, nullptr, 0, nullptr,
		1, &barrier);

}

void Sampler::AllocateImage(VkMemoryPropertyFlags memoryProperty) {
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_device->GetVkDevice(), m_image, &memoryRequirements);

	const VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		// TODO: Learn more about this
		.memoryTypeIndex = this->FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperty)
	};

	// TODO: Learn about VMA
	const VkResult result = vkAllocateMemory(m_device->GetVkDevice(), &memoryAllocateInfo, nullptr, &m_imageMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Sampler] Could not allocate memory for the vertex buffer");
	}

	m_allocatedMemorySize = memoryRequirements.size;
	vkBindImageMemory(m_device->GetVkDevice(), m_image, m_imageMemory, 0);
}

uint32_t Sampler::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_device->GetVkPhysicalDevice(), &memoryProperties);

	for (uint32_t ind = 0; ind < memoryProperties.memoryTypeCount; ind++) {
		if (typeFilter & (1 << ind) && (memoryProperties.memoryTypes[ind].propertyFlags & properties)) {
			return ind;
		}
	}

	throw std::runtime_error("[Sampler] Could not find correct memory type");
}

