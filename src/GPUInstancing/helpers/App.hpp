#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

#include "ShaderManager.hpp"

class App {
public:
	void Start();

private:
	void InitializeImGUI();
	void InitializeVulkan();
	void DestroyVulkan() const;

	void Update();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateSurface();
	void PickGPU();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	bool IsGPUSupported(const VkPhysicalDevice physicalDevice);
	void FindFamilyQueues(const VkPhysicalDevice physicalDevice);
	bool DoesDeviceSupportExtensions(const VkPhysicalDevice physicalDevice) const;

	struct SwapChainSupportInfo;
	SwapChainSupportInfo QuerySwapChainSupport(VkPhysicalDevice physicalDevice) const;

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	const char* const* GetVulkanValidationLayers(uint32_t& layerCount) const;
	const char* const* GetVulkanInstanceExtensions(uint32_t& extensionCount) const;

	GLFWwindow* glfwWindow_ = nullptr;

	std::unique_ptr<ShaderManager> shaderManager_;

	VkInstance vkInstance_ = VK_NULL_HANDLE;
	VkPhysicalDevice vkPhysicalDevice_ = VK_NULL_HANDLE;
	VkDevice vkLogicalDevice_ = VK_NULL_HANDLE;
	VkSurfaceKHR vkSurface_ = VK_NULL_HANDLE;
	VkPipeline vkGraphicsPipeline_ = VK_NULL_HANDLE;

	VkSwapchainKHR vkSwapChain_ = VK_NULL_HANDLE;
	std::vector<VkImage> vkSwapChainImages_{};
	std::vector<VkImageView> vkSwapChainImageViews_{};
	std::vector<VkFramebuffer> vkSwapChainFramebuffers_{};

	VkFormat vkSwapChainImageFormat_{};
	VkExtent2D vkSwapChainExtent_{};

	VkRenderPass vkRenderPass_ = VK_NULL_HANDLE;
	VkPipelineLayout vkPipelineLayout_ = VK_NULL_HANDLE;

	VkCommandPool vkGraphicsCommandPool_ = VK_NULL_HANDLE;
	VkCommandBuffer vkGraphicsCommandBuffer_ = VK_NULL_HANDLE;

	VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
	VkFence inFlightFence_ = VK_NULL_HANDLE;

	/**
	 * Currently, represents both graphics and presentation queue.
	 */
	VkQueue graphicsQueue_ = VK_NULL_HANDLE;
	std::optional<uint32_t> graphicsQueueIndex_{};
	float graphicsQueuePriority_{ 1.0f };

	/* * *
	 * Config
	 *
	 * TODO:
	 * 1. Implement separation of presentation and graphics queue.
	 */
	std::vector<const char*> vkValidationLayers_ = {
		"VK_LAYER_KHRONOS_validation"
	};
	std::vector<const char*> vkDeviceExtensions_ = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkPhysicalDeviceType vkRequiredDeviceType_ = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	uint32_t swapChainImageCount_ = 2;
	uint32_t framesInFlightCount_ = 2;


	/* * *
	 * Inner classes and structs
	 */

	/**
	 * WIP
	 */
	struct QueueInfo
	{
		VkQueue vkQueue = VK_NULL_HANDLE;
		std::optional<uint32_t> familyIndex{};
		float priority{ 1.0f };
	};

	struct SwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
};
