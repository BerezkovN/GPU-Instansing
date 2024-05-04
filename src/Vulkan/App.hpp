#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

#include "Config.hpp"
#include "helpers/Device.hpp"
#include "helpers/ShaderManager.hpp"
#include "helpers/DeviceQueue.hpp"
#include "helpers/Surface.hpp"
#include "helpers/Swapchain.hpp"
#include "helpers/IRenderPass.hpp"
#include "helpers/IRenderPipeline.hpp"

class App {
public:

    App();
	void Run();
    void Destroy();

	void HintWindowResize();

	void GetScreenSize(int& width, int& height) const;

	void SetSwapchainImageCount(uint32_t count) const;
	[[nodiscard]] uint32_t GetSwapchainImageCount() const;
    [[nodiscard]] const Config* GetConfig() const;
    [[nodiscard]] VkInstance GetVkInstance() const;


private:

    void InitializeWindow();
    void DestroyWindow();

    void InitializeVulkan();
    void DestroyVulkan();

    void ChooseMainDevice();

	void Update();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


	void ResizeImmediately();

	void CreateFramebuffers();
	void DestroyFramebuffers();

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();

    [[nodiscard]] const char* const* GetVulkanInstanceExtensions(uint32_t& extensionCount) const;
    [[nodiscard]] const char* const* GetVulkanValidationLayers(uint32_t& layerCount) const;

    GLFWwindow* m_window = nullptr;
    VkInstance m_instance;

    DeviceID m_mainDeviceID;
    std::unique_ptr<Device> m_mainDevice;

    std::unique_ptr<Surface> m_surface;

	bool m_mustResize{};
	std::unique_ptr<Swapchain> m_swapchain;
	std::vector<VkFramebuffer> m_framebuffers;

	std::unique_ptr<ShaderManager> m_shaderManager;
	std::unique_ptr<IRenderPass> m_renderPass;

	std::unique_ptr<IRenderPipeline> m_renderPipeline;

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
	std::shared_ptr<DeviceQueue> m_graphicsQueue;

	/* * *
	 * TODO:
	 * 1. Implement separation of presentation and graphics queue.
	 */

    std::unique_ptr<Config> m_config;


};
