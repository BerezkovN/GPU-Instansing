#include "StagingBuffer.hpp"

#include <vector>

StagingBuffer::StagingBuffer(const Device* device, const Desc& desc) : GenericBuffer(device) {

    std::vector<uint32_t> queueFamilyIndices;
    VkSharingMode sharingMode;

    if (!desc.transferQueue.has_value() || desc.graphicsQueue->GetFamilyIndex() == desc.transferQueue.value()->GetFamilyIndex()) {
        queueFamilyIndices.push_back(desc.graphicsQueue->GetFamilyIndex());
        sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else {
        queueFamilyIndices.push_back(desc.graphicsQueue->GetFamilyIndex());
        queueFamilyIndices.push_back(desc.transferQueue.value()->GetFamilyIndex());

        sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    const VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = desc.bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
        .pQueueFamilyIndices = queueFamilyIndices.data()
    };

    this->CreateBuffer(createInfo);
    this->AllocateBuffer(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
