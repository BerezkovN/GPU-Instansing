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
	void HintWindowResize();

private:
	void InitializeImGUI();
	void InitializeVulkan();
	void DestroyVulkan();
	void DestroySwapchain();

	void Update();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateSurface();
	void PickGPU();
	void CreateLogicalDevice();

	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void RecreateSwapChain();

	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	bool IsGPUSupported(VkPhysicalDevice physicalDevice);
	void FindFamilyQueues(VkPhysicalDevice physicalDevice);
	bool DoesDeviceSupportExtensions(VkPhysicalDevice physicalDevice) const;

	struct SwapChainSupportInfo;
	SwapChainSupportInfo QuerySwapChainSupport(VkPhysicalDevice physicalDevice) const;

	[[nodiscard]] VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
	[[nodiscard]] VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
	[[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

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

	bool resizeFramebuffer_{};

	VkFormat vkSwapChainImageFormat_{};
	VkExtent2D vkSwapChainExtent_{};

	VkRenderPass vkRenderPass_ = VK_NULL_HANDLE;
	VkPipelineLayout vkPipelineLayout_ = VK_NULL_HANDLE;

	VkCommandPool vkGraphicsCommandPool_ = VK_NULL_HANDLE;

	/* * *
	 * Per frame-in-flight objects
	 */

	std::vector<VkCommandBuffer> graphicsCommandBuffers_{};
	std::vector<VkSemaphore> imageAvailableSemaphores_{};
	std::vector<VkSemaphore> renderFinishedSemaphores_{};
	std::vector<VkFence> inFlightFences_{};

	uint32_t currentFrameInFlight_{};

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
	uint32_t framesInFlightCount_ = 1;


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
