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

	void* MapMemory(const VkDeviceSize memorySize);
	void UnmapMemory();

	void CopyData(const void* data, size_t dataSize);
	void CopyFromBuffer(VkCommandBuffer commandBuffer, const GenericBuffer* srcBuffer, const VkBufferCopy& bufferCopyInfo) const;

	[[nodiscard]] void* GetMappedMemory() const;
	[[nodiscard]] VkDeviceSize GetAllocatedMemorySize() const;
	[[nodiscard]] VkDeviceSize GetBufferSize() const;
	[[nodiscard]] VkBuffer GetVkBuffer() const;
	[[nodiscard]] VkDeviceMemory GetVkDeviceMemory() const;


protected:

	GenericBuffer(const Device* device);

	void CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo);
	void AllocateBuffer(VkMemoryPropertyFlags memoryProperty);

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	const Device* m_device;

	VkBuffer m_buffer;
	VkDeviceMemory m_bufferMemory;

	// These could be sometimes different because of the memory requirements.
	VkDeviceSize m_bufferSize = 0;
	VkDeviceSize m_allocatedMemorySize = 0;

	void* m_mappedMemory = nullptr;
};