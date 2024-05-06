#pragma once

#include "GenericBuffer.hpp"

class StagingBuffer : public GenericBuffer
{
public:
	struct Desc
	{
		const DeviceQueue* graphicsQueue;
		std::optional<const DeviceQueue*> transferQueue;
		uint32_t bufferSize;
	};

	StagingBuffer(const Device* device, const StagingBuffer::Desc& desc);
}; 