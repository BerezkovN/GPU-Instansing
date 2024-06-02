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
	if (ToGigaBytes(size) > 1.0) {
		return std::format("{:.2f} GB", ToGigaBytes(size));
	}

	return std::format("{:.2f} MB", ToMegaBytes(size));
}

DeviceMemory::DeviceMemory(const Device* device) {

	m_device = device;

	this->LogHeapInfo();
	this->LogMemoryRequirements();
}

DeviceMemory::~DeviceMemory() {

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

void DeviceMemory::LogMemoryRequirements() {

}
