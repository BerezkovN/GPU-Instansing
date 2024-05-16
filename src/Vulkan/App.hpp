#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

#include "Config.hpp"
#include "helpers/Device.hpp"
#include "helpers/ShaderManager.hpp"
#include "helpers/DeviceQueue.hpp"
#include "helpers/IRenderer.hpp"
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

	void ResizeImmediately();

	void CreateFramebuffers();
	void DestroyFramebuffers();

	void CreateSyncObjects();
	void DestroySyncObjects();

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

	std::unique_ptr<IRenderPass> m_renderPass;

	std::unique_ptr<IRenderer> m_renderer;

	/* * *
	 * Frame synchronization objects.
	 */

	VkSemaphore m_imageAvailableSemaphore{};
	VkSemaphore m_renderFinishedSemaphore{};
	VkFence m_submitFrameFence{};

	std::shared_ptr<DeviceQueue> m_graphicsQueue;
    std::optional<std::shared_ptr<DeviceQueue>> m_transferQueue;

    std::unique_ptr<Config> m_config;
};
