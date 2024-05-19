#pragma once

#include "GenericBuffer.hpp"

class Context;

class StagingBuffer : public GenericBuffer
{
public:
	StagingBuffer(const Context* context, VkDeviceSize bufferSize);

	void CopyData(const void* data, size_t dataSize) override;
}; 