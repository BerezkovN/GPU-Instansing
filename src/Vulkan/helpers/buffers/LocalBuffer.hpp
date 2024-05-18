#pragma once

#include "GenericBuffer.hpp"

class LocalBuffer : public GenericBuffer
{
public:
	struct Desc
	{
		const DeviceQueue* graphicsQueue;
		std::optional<const DeviceQueue*> transferQueue;
		VkCommandBuffer transferCommandBuffer;

		VkBufferUsageFlags usageFlags;

		const void* buffer;
		VkDeviceSize bufferSize;
	};

	LocalBuffer(const Device* device, const LocalBuffer::Desc& desc);
};