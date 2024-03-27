#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

class App {
public:
	void Start();

private:
	void InitializeVulkan();
	void DestroyVulkan() const;

	void Update();

	void PickGPU();

	bool IsGPUSupported(const VkPhysicalDevice physicalDevice) const;
	std::optional<uint32_t> FindFamilyQueue(const VkPhysicalDevice physicalDevice) const;

	const char* const* GetVulkanValidationLayers(uint32_t& layerCount) const;
	const char* const* GetVulkanExtensions(uint32_t& extensionCount) const;

	GLFWwindow* glfwWindow_ = nullptr;
	VkInstance vkInstance_ = nullptr;
	VkPhysicalDevice vkPhysicalDevice_ = VK_NULL_HANDLE;

	std::vector<const char*> vkValidationLayers_ = {
		"VK_LAYER_KHRONOS_validation"
	};
	VkPhysicalDeviceType vkRequiredDeviceType_ = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	VkQueueFlags vkRequiredQueueFlags_ = VK_QUEUE_GRAPHICS_BIT;
};
