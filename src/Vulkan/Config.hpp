#pragma once

#include "pch.hpp"
#include "helpers/Device.hpp"

struct Config {
    std::vector<const char*> vkValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> vkDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    std::optional<DeviceID> mainDeviceID{ 13636323128864058671u };
    VkPhysicalDeviceType vkRequiredDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    VkFormat vkPreferredSurfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR vkPreferredSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    VkPresentModeKHR vkPreferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    uint32_t swapChainImageCount = 2;
    uint32_t framesInFlight = 2;
};