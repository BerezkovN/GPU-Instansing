#include "LocalBuffer.hpp"

#include "StagingBuffer.hpp"
#include "../Context.hpp"

LocalBuffer::LocalBuffer(const Context* context, const LocalBuffer::Desc& desc) : GenericBuffer(context) {


    StagingBuffer stagingBuffer(m_context, desc.bufferSize);
    stagingBuffer.CopyData(desc.buffer, desc.bufferSize);

    this->CreateBuffer({
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = desc.bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | desc.usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    });
    this->AllocateBuffer(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCommandBuffer transferCommandBuffer = m_context->GetTransferCommandBuffer();
	this->CopyFromBuffer(transferCommandBuffer, &stagingBuffer, {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = desc.bufferSize
    });

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &transferCommandBuffer
    };

    const VkQueue copyQueue = m_context->GetActualTransferQueue()->GetVkQueue();
    vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(copyQueue);

    stagingBuffer.Destroy();
}
