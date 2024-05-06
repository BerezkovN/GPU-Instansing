#include "GenericBuffer.hpp"

#include <stdexcept>

GenericBuffer::GenericBuffer(const Device* device, const GenericBuffer::Desc& desc) {

    m_device = device;

    this->CreateBuffer(desc.bufferCreateInfo);
    this->AllocateBuffer(desc.memoryProperty);
}

void GenericBuffer::Destroy() {
    vkDestroyBuffer(m_device->GetVkDevice(), m_buffer, nullptr);
    vkFreeMemory(m_device->GetVkDevice(), m_bufferMemory, nullptr);

    m_buffer = VK_NULL_HANDLE;
    m_bufferMemory = VK_NULL_HANDLE;
}

void GenericBuffer::CopyData(const void* data, size_t dataSize) const {

    void* mappedPointer;
    vkMapMemory(m_device->GetVkDevice(), m_bufferMemory, 0, dataSize, 0, &mappedPointer);
    std::memcpy(mappedPointer, data, dataSize);
    vkUnmapMemory(m_device->GetVkDevice(), m_bufferMemory);
}

void GenericBuffer::CopyFromBuffer(const VkCommandBuffer commandBuffer, const GenericBuffer* srcBuffer, const VkBufferCopy& bufferCopyInfo) const {

	constexpr VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    vkCmdCopyBuffer(commandBuffer, srcBuffer->GetVkBuffer(), m_buffer, 1, &bufferCopyInfo);
    vkEndCommandBuffer(commandBuffer);
}

VkBuffer GenericBuffer::GetVkBuffer() const {
    return m_buffer;
}

VkDeviceMemory GenericBuffer::GetVkDeviceMemory() const {
    return m_bufferMemory;
}

GenericBuffer::GenericBuffer(const Device* device) {

	m_device = device;

    m_buffer = VK_NULL_HANDLE;
    m_bufferMemory = VK_NULL_HANDLE;
}

void GenericBuffer::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo) {

	const VkResult result = vkCreateBuffer(m_device->GetVkDevice(), &bufferCreateInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[GenericBuffer] Could not create buffer");
    }
}

void GenericBuffer::AllocateBuffer(VkMemoryPropertyFlags memoryProperty) {

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_device->GetVkDevice(), m_buffer, &memoryRequirements);

    const VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        // TODO: Learn more about this
        .memoryTypeIndex = this->FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperty)
    };

    // TODO: Learn about VMA
    const VkResult result = vkAllocateMemory(m_device->GetVkDevice(), &memoryAllocateInfo, nullptr, &m_bufferMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[GenericBuffer] Could not allocate memory for the vertex buffer");
    }

    vkBindBufferMemory(m_device->GetVkDevice(), m_buffer, m_bufferMemory, 0);
}

uint32_t GenericBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device->GetVkPhysicalDevice(), &memoryProperties);

    for (uint32_t ind = 0; ind < memoryProperties.memoryTypeCount; ind++) {
        if (typeFilter & (1 << ind) && (memoryProperties.memoryTypes[ind].propertyFlags & properties)) {
            return ind;
        }
    }

    throw std::runtime_error("[GenericBuffer] Could not find correct memory type");
}

