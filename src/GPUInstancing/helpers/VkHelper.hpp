#pragma once

#include <string>
#include <vulkan/vulkan.h>

inline std::string vk_to_string(VkPhysicalDeviceType deviceType);

inline std::string vk_to_string(const VkQueueFlags queueFlags);
