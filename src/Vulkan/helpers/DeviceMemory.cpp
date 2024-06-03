#include "DeviceMemory.hpp"

#include "../pch.hpp"

#include "Device.hpp"
#include "VkHelper.hpp"



double ToGigaBytes(VkDeviceSize size) {
	return static_cast<double>(size) / 1024.0 / 1024.0 / 1024.0;
}

double ToMegaBytes(VkDeviceSize size) {
	return static_cast<double>(size) / 1024.0 / 1024.0;
}

std::string ToBestRepresentation(VkDeviceSize size) {

	double castedSize = static_cast<double>(size);

	castedSize /= 1024.0;
	if (castedSize < 1024.0) {
		return std::format("{:.2f} KB", castedSize);
	}

	castedSize /= 1024.0;
	if (castedSize < 1024.0) {
		return std::format("{:.2f} MB", castedSize);
	}

	castedSize /= 1024.0;
	return std::format("{:.2f} GB", castedSize);
}

DeviceMemory::DeviceMemory(const Device* device) {

	m_device = device;

	this->LogHeapInfo();
	this->LogMemoryRequirements();
}

DeviceMemory::~DeviceMemory() {

	if (!m_allocatedMemory.empty()) {
		spdlog::error("[DeviceMemory] Detected memory leak. You must deallocate {} more resources", m_allocatedMemory.size());
	}
}

VkDeviceMemory DeviceMemory::AllocateMemory(const DeviceMemory::AllocationDesc& desc) {

	const VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = desc.memoryRequirements.size,
		.memoryTypeIndex = this->FindMemoryType(desc.memoryRequirements.memoryTypeBits, desc.memoryPropertyFlags)
	};

	VkDeviceMemory memory;
	
	const VkResult result = vkAllocateMemory(m_device->GetVkDevice(), &memoryAllocateInfo, nullptr, &memory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[DeviceMemory] Could not allocate memory");
	}

	m_allocatedMemory.insert(memory);
	return memory;
}

void DeviceMemory::FreeMemory(VkDeviceMemory memory) {

	vkFreeMemory(m_device->GetVkDevice(), memory, nullptr);
	m_allocatedMemory.erase(memory);
}

uint32_t DeviceMemory::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_device->GetVkPhysicalDevice(), &memoryProperties);

	for (uint32_t ind = 0; ind < memoryProperties.memoryTypeCount; ind++) {
		if (typeFilter & (1 << ind) && ((memoryProperties.memoryTypes[ind].propertyFlags & properties) == properties)) {
			return ind;
		}
	}

	throw std::runtime_error("[DeviceMemory] Could not find correct memory type");
}


void DeviceMemory::LogHeapInfo() const {

	if (!m_device->IsExtensionEnabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
		spdlog::warn("[DeviceMemory] Heap info is unavailable");
		return;
	}

	VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudget = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
	};

	VkPhysicalDeviceMemoryProperties2 memoryProperties2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
		.pNext = &memoryBudget,
	};

	vkGetPhysicalDeviceMemoryProperties2(m_device->GetVkPhysicalDevice(), &memoryProperties2);

	const auto& memoryProperties = memoryProperties2.memoryProperties;

	spdlog::info("[DeviceMemory] Heap info:");
	for (uint32_t heapInd = 0; heapInd < memoryProperties.memoryHeapCount; heapInd++) {

		VkMemoryHeap heap = memoryProperties.memoryHeaps[heapInd];

		spdlog::info("[DeviceMemory] \tHeap {}: {} B ({}) {}", 
			heapInd,
			heap.size, ToBestRepresentation(heap.size),
			VkHelper::MemoryHeapFlagsToString(heap.flags, ","));

		spdlog::info("[DeviceMemory] \t\theapBudget = {} B ({:.2f} MB)", memoryBudget.heapBudget[heapInd], ToMegaBytes(memoryBudget.heapBudget[heapInd]));
		spdlog::info("[DeviceMemory] \t\theapUsage = {} B ({:.2f} MB)", memoryBudget.heapUsage[heapInd], ToMegaBytes(memoryBudget.heapUsage[heapInd]));

		for (uint32_t typeInd = 0; typeInd < memoryProperties.memoryTypeCount; typeInd++) {

			if (memoryProperties.memoryTypes[typeInd].heapIndex != heapInd) {
				continue;
			}

			if (memoryProperties.memoryTypes[typeInd].propertyFlags == 0) {
				continue;
			}

			spdlog::info("[DeviceMemory] \t\tType {}: {}", typeInd, VkHelper::MemoryPropertyFlagsToString(memoryProperties.memoryTypes[typeInd].propertyFlags));
		}

	}
	spdlog::info("");
}

void DeviceMemory::LogMemoryRequirements() const {

	const auto& physicalDeviceProperties = m_device->GetVkPhysicalDeviceProperties();
	
	spdlog::info("[DeviceMemory] Limits:");
	spdlog::info("[DeviceMemory] Push constants size: {}", physicalDeviceProperties.limits.maxMemoryAllocationCount);
	spdlog::info("[DeviceMemory] Min uniform buffer offset alignment: {}", physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
	spdlog::info("[DeviceMemory] Min storage buffer offset alignment: {}", physicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
	spdlog::info("[DeviceMemory] Min map memory alignment: {}", physicalDeviceProperties.limits.minMemoryMapAlignment);
	spdlog::info("[DeviceMemory] Optimal buffer copy offset alignment: {}", physicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment);
	spdlog::info("[DeviceMemory] Optimal buffer copy row pitch alignment: {}", physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment);
	spdlog::info("");
}
