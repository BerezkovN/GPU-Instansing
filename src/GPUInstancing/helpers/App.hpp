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

    void GetScreenSize(int& width, int& height) const;

	[[nodiscard]] VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
	[[nodiscard]] VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
	[[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	const char* const* GetVulkanValidationLayers(uint32_t& layerCount) const;
	const char* const* GetVulkanInstanceExtensions(uint32_t& extensionCount) const;

	GLFWwindow* m_window = nullptr;

	std::unique_ptr<ShaderManager> m_shaderManager;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_logicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapChainImages{};
	std::vector<VkImageView> m_swapChainImageViews{};
	std::vector<VkFramebuffer> m_swapChainFramebuffers{};

	bool m_mustResize{};

	VkFormat m_swapChainImageFormat{};
	VkExtent2D m_swapChainExtent{};

	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;

	/* * *
	 * Per frame-in-flight objects
	 */

	std::vector<VkCommandBuffer> m_graphicsCommandBuffers{};
	std::vector<VkSemaphore> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores{};
	std::vector<VkFence> m_inFlightFences{};

	uint32_t m_currentFrameInFlight{};

	/**
	 * Currently, represents both graphics and presentation queue.
	 */
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	std::optional<uint32_t> m_graphicsQueueIndex{};
	float m_graphicsQueuePriority{1.0f };

	/* * *
	 * Config
	 *
	 * TODO:
	 * 1. Implement separation of presentation and graphics queue.
	 */

    struct Config {
        std::vector<const char*> vkValidationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char*> vkDeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        VkPhysicalDeviceType vkRequiredDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        VkFormat vkPreferredSurfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
        VkColorSpaceKHR vkPreferredSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        VkPresentModeKHR vkPreferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

        uint32_t swapChainImageCount = 2;
        uint32_t framesInFlight = 2;
    } m_config;


	struct SwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
};
