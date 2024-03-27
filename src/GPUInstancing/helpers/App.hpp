#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <functional>

class App {
public:
	void Start();

private:
	void InitializeVulkan();
	void DestroyVulkan() const;

	void Update();

	void CreateSurface();
	void PickGPU();
	void CreateLogicalDevice();
	void CreateSwapChain();

	bool IsGPUSupported(const VkPhysicalDevice physicalDevice);
	void FindFamilyQueues(const VkPhysicalDevice physicalDevice);
	bool DoesSupportDeviceExtensions(const VkPhysicalDevice physicalDevice) const;

	struct SwapChainSupportInfo;
	SwapChainSupportInfo QuerySwapChainSupport(VkPhysicalDevice physicalDevice) const;

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	const char* const* GetVulkanValidationLayers(uint32_t& layerCount) const;
	const char* const* GetVulkanExtensions(uint32_t& extensionCount) const;

	GLFWwindow* glfwWindow_ = nullptr;
	VkInstance vkInstance_ = nullptr;
	VkPhysicalDevice vkPhysicalDevice_ = VK_NULL_HANDLE;
	VkDevice vkLogicalDevice_ = VK_NULL_HANDLE;
	VkSurfaceKHR vkSurface_ = VK_NULL_HANDLE;

	/**
	 * Guaranteed to have surface support.
	 */
	VkQueue graphicsQueue_ = VK_NULL_HANDLE;
	std::optional<uint32_t> graphicsQueueIndex_{};
	float graphicsQueuePriority_{ 1.0f };


	// TODO: Make this configurable through config or smth.
	std::vector<const char*> vkValidationLayers_ = {
		"VK_LAYER_KHRONOS_validation"
	};
	std::vector<const char*> vkDeviceExtensions_ = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkPhysicalDeviceType vkRequiredDeviceType_ = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	uint32_t swapChainImageCount_ = 2;

	struct SwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
};
