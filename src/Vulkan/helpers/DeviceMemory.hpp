#pragma once

#include <volk.h>
#include <unordered_set>

class Device;

class DeviceMemory
{
public:
	DeviceMemory(const Device* device);
	~DeviceMemory();

	VkDeviceMemory AllocateMemory(VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlags propertyFlags);
	void FreeMemory(VkDeviceMemory memory);

private:
	void LogHeapInfo() const;
	void LogMemoryRequirements();

	const Device* m_device;

	std::unordered_set<VkDeviceMemory> m_allocatedMemory;
};