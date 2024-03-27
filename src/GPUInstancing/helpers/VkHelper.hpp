#pragma once

#include <string>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

inline std::string vk_to_string(VkPhysicalDeviceType deviceType);

inline std::string vk_to_string(const VkQueueFlags queueFlags);

/**
 * Checks if all required names are found in present names.
 * This is useful for checking if all required extensions are supported by a physical device,
 * and many more similar cases.
 *
 * @tparam T Custom type
 * @param requiredNames 
 * @param presentNames 
 * @param accessor Sometimes, you don't have a simple vector of string, but rather an object with a field.
 *  This allows you to check more complicated structures.
 */
template<typename T>
bool does_contain_names(
    const std::vector<const char*>& requiredNames, 
    const std::vector<T>& presentNames, 
    std::function<const char* (const T&)> accessor)
{
    for (const auto requiredName : requiredNames) {

        bool wasNameFound = false;
        for (const auto& presentName : presentNames) {

            if (strcmp(requiredName, accessor(presentName)) != 0) {
                wasNameFound = true;
                break;
            }
        }

        if (!wasNameFound) {
            return false;
        }
    }

    return true;
}
