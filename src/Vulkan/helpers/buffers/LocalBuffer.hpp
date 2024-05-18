#pragma once

#include "GenericBuffer.hpp"

class LocalBuffer : public GenericBuffer
{
public:
	struct Desc
	{
		VkBufferUsageFlags usageFlags;

		const void* buffer;
		VkDeviceSize bufferSize;
	};

	LocalBuffer(const Context* context, const LocalBuffer::Desc& desc);
};