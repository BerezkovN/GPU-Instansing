#pragma once

#include <volk.h>

#include "../Device.hpp"

class GenericBuffer
{
public:
	struct Desc
	{
		VkBufferCreateInfo bufferCreateInfo;
		VkMemoryPropertyFlags memoryProperty;
	};

	GenericBuffer(const Device* device, const GenericBuffer::Desc& desc);
	void Destroy();

	void CopyData(const void* data, size_t dataSize) const;
	void CopyFromBuffer(VkCommandBuffer commandBuffer, const GenericBuffer* srcBuffer, const VkBufferCopy& bufferCopyInfo) const;

	[[nodiscard]] VkBuffer GetVkBuffer() const;
	[[nodiscard]] VkDeviceMemory GetVkDeviceMemory() const;


private:

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	const Device* m_device;

	VkBuffer m_buffer;
	VkDeviceMemory m_bufferMemory;
};