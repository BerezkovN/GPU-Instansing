#pragma once

#include <volk.h>
#include <unordered_set>

class Device;

class DeviceMemory
{
public:
	DeviceMemory(const Device* device);
	~DeviceMemory();

	struct AllocationDesc
	{
		VkMemoryRequirements memoryRequirements;
		VkMemoryPropertyFlags memoryPropertyFlags;
	};

	[[nodiscard]] VkDeviceMemory AllocateAndBindMemory(const DeviceMemory::AllocationDesc& desc);
	void FreeMemory(VkDeviceMemory memory);

	[[nodiscard]] bool IsBARSupported();

private:
	[[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	void LogHeapInfo() const;
	void LogMemoryRequirements();

	const Device* m_device;

	std::unordered_set<VkDeviceMemory> m_allocatedMemory;
};