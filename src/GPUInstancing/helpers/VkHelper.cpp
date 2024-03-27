#include "VkHelper.hpp"

inline std::string vk_to_string(const VkPhysicalDeviceType deviceType) {
	switch (deviceType)  // NOLINT(clang-diagnostic-switch-enum)
	{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			return "Other";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "Integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "Discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "Virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "CPU";
		default:
			return "Unknown";
	}
}

std::string vk_to_string(const VkQueueFlags queueFlags) {

	std::string result("{");

	if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
		result += " Graphics ";
	}
	if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
		result += "| Compute ";
	}
	if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
		result += "| Transfer ";
	}
	if (queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
		result += "| Sparse binding ";
	}
	if (queueFlags & VK_QUEUE_PROTECTED_BIT) {
		result += "| Protected ";
	}
	if (queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) {
		result += "| Optical flow nv ";
	}

	result += '}';

	return result;
}
