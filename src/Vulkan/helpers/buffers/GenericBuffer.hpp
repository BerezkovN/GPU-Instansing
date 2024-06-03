#pragma once

#include <volk.h>

class Context;

class GenericBuffer
{
public:
	struct Desc
	{
		VkBufferCreateInfo bufferCreateInfo;
		VkMemoryPropertyFlags memoryProperty;
	};

	GenericBuffer(const Context* context, const GenericBuffer::Desc& desc);
	void Destroy();

	void* MapMemory(const VkDeviceSize memorySize);
	void UnmapMemory();

	virtual void CopyData(const void* data, size_t dataSize);
	void CopyFromBuffer(VkCommandBuffer commandBuffer, const GenericBuffer* srcBuffer, const VkBufferCopy& bufferCopyInfo) const;

	[[nodiscard]] void* GetMappedMemory() const;

	/**
	 * Amount of memory in bytes that system actually allocated
	 */
	[[nodiscard]] VkDeviceSize GetAllocatedMemorySize() const;

	/**
	 * Amount of memory in bytes that we asked for when creating the buffer.
	 */
	[[nodiscard]] VkDeviceSize GetBufferSize() const;
	[[nodiscard]] VkBuffer GetVkBuffer() const;
	[[nodiscard]] VkDeviceMemory GetVkDeviceMemory() const;


protected:

	explicit GenericBuffer(const Context* context);

	void CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo);
	void AllocateBuffer(VkMemoryPropertyFlags memoryPropertyFlags);

	const Context* m_context{};

	VkBuffer m_buffer{};
	VkBufferUsageFlags m_bufferUsage{};
	VkDeviceMemory m_bufferMemory{};

	// These could be sometimes different because of the memory requirements.
	VkDeviceSize m_bufferSize = 0;
	VkDeviceSize m_allocatedMemorySize = 0;

	void* m_mappedMemory = nullptr;
};