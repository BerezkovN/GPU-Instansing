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
    this->AllocateBuffer(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
}

void StagingBuffer::CopyData(const void* data, size_t dataSize) {

    void* mappedPointer = this->MapMemory(dataSize);
    std::memcpy(mappedPointer, data, dataSize);

	const VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = m_bufferMemory,
        .offset = 0,
        .size = dataSize
    };
    vkFlushMappedMemoryRanges(m_context->GetDevice()->GetVkDevice(), 1, &range);

    this->UnmapMemory();
}
