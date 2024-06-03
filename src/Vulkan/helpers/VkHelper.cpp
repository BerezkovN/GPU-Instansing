#include "VkHelper.hpp"

#include <sstream>

// Macro to check flag and add to result string
#define ADD_FLAG(flag, flag_name) \
if (flags & (flag)) { \
    if (!first) { \
        result << (separator); \
    } \
    result << " " << (flag_name); \
	flags &= ~(flag); \
    first = false; \
}

std::string VkHelper::DeviceTypeToString(VkPhysicalDeviceType deviceType) {
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

std::string VkHelper::QueueFlagsToString(VkQueueFlags flags, const char* separator) {

	std::ostringstream result;
	bool first = true;

	ADD_FLAG(VK_QUEUE_GRAPHICS_BIT, "GRAPHICS")
	ADD_FLAG(VK_QUEUE_COMPUTE_BIT, "COMPUTE")
	ADD_FLAG(VK_QUEUE_TRANSFER_BIT, "TRANSFER")
	ADD_FLAG(VK_QUEUE_SPARSE_BINDING_BIT, "SPARSE_BINDING")
	ADD_FLAG(VK_QUEUE_PROTECTED_BIT, "PROTECTED")

	if (flags != 0) {
		result << separator << " UNKNOWN (" << flags << ")";
	}

	return result.str();
}

std::string VkHelper::MemoryHeapFlagsToString(VkMemoryHeapFlags flags, const char* separator) {
	std::ostringstream result;
	bool first = true;

	ADD_FLAG(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, "DEVICE_LOCAL")
	ADD_FLAG(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT, "MULTI_INSTAMCE")

	if (flags != 0) {
		result << separator << " UNKNOWN (" << flags << ")";
	}

	return result.str();
}

std::string VkHelper::MemoryPropertyFlagsToString(VkMemoryPropertyFlags flags , const char* separator) {
	std::ostringstream result;
	bool first = true;

	ADD_FLAG(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "DEVICE_LOCAL")
	ADD_FLAG(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "HOST_VISIBLE")
	ADD_FLAG(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "HOST_COHERENT")
	ADD_FLAG(VK_MEMORY_PROPERTY_HOST_CACHED_BIT, "HOST_CACHED")
	ADD_FLAG(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, "LAZILY_ALLOCATED")
	ADD_FLAG(VK_MEMORY_PROPERTY_PROTECTED_BIT, "PROTECTED")
	ADD_FLAG(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD, "DEVICE_COHERENT_AMD")
	ADD_FLAG(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD, "DEVICE_UNCACHED_AMD")
	ADD_FLAG(VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV, "RDMA_CAPABLE_NV")

	if (flags != 0) {
		result << separator << " UNKNOWN (" << flags << ")";
	}

	return result.str();
}

std::string VkHelper::BufferUsageFlagsToString(VkBufferUsageFlags flags, const char* separator) {
	std::ostringstream result;
	bool first = true;

	ADD_FLAG(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "TRANSFER_SRC");
	ADD_FLAG(VK_BUFFER_USAGE_TRANSFER_DST_BIT, "TRANSFER_DST");
	ADD_FLAG(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, "UNIFORM_TEXEL_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, "STORAGE_TEXEL_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "UNIFORM_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "STORAGE_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "INDEX_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "VERTEX_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, "INDIRECT_BUFFER");
	ADD_FLAG(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, "SHADER_DEVICE_ADDRESS");

	if (flags != 0) {
		result << separator << " UNKNOWN (" << flags << ")";
	}

	return result.str();
}
