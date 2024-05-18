#include "LocalBuffer.hpp"

#include "StagingBuffer.hpp"

LocalBuffer::LocalBuffer(const Device* device, const LocalBuffer::Desc& desc) : GenericBuffer(device) {

	const StagingBuffer::Desc stagingBufferDesc = {
        .graphicsQueue = desc.graphicsQueue,
        .transferQueue = desc.transferQueue,
        .bufferSize = desc.bufferSize
    };

    StagingBuffer stagingBuffer(m_device, stagingBufferDesc);
    stagingBuffer.CopyData(desc.buffer, desc.bufferSize);

    this->CreateBuffer({
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = desc.bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | desc.usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    });
    this->AllocateBuffer(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	this->CopyFromBuffer(desc.transferCommandBuffer, &stagingBuffer, {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = desc.bufferSize
    });

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &desc.transferCommandBuffer
    };

    const VkQueue copyQueue = desc.transferQueue.has_value() ? desc.transferQueue.value()->GetVkQueue() : desc.graphicsQueue->GetVkQueue();
    vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(copyQueue);

    stagingBuffer.Destroy();
}
