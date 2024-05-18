#pragma once

#include "GenericBuffer.hpp"

class Context;

class StagingBuffer : public GenericBuffer
{
public:
	StagingBuffer(const Context* context, VkDeviceSize bufferSize);
}; 