#include "StagingBuffer.hpp"

#include <vector>

#include "../Context.hpp"

StagingBuffer::StagingBuffer(const Context* context, const VkDeviceSize bufferSize) : GenericBuffer(context) {

    Context::ShareInfo shareInfo = context->GetTransferShareInfo();

    const VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = shareInfo.sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(shareInfo.queueFamilyIndices.size()),
        .pQueueFamilyIndices = shareInfo.queueFamilyIndices.data()
    };

    this->CreateBuffer(createInfo);
    this->AllocateBuffer(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
