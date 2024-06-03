#pragma once

#include <volk.h>
#include <cstdint>
#include <functional>
#include <GLFW/glfw3.h>

#include "Device.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"

class Swapchain;
class IRenderPass;
class IComponentSystem;
class IRenderer;


class Context
{
    // Helps hiding some fields from being exposed to the API.
    friend class Device;

public:

    struct Config
    {
        std::vector<const char*> vkValidationLayers;
        std::vector<const char*> vkDeviceExtensions;
        std::optional<DeviceID> mainDeviceId;

    	VkFormat vkPreferredSurfaceFormat;
        VkColorSpaceKHR vkPreferredSurfaceColorSpace;

        VkPresentModeKHR vkPreferredPresentMode;
        uint32_t swapChainImageCount;

        bool useImGui;
    };

    struct CreateDesc
    {
        std::shared_ptr<Context::Config> config;
        IRenderPass* renderPass;
        IComponentSystem* componentSystem;
    };

    explicit Context(const CreateDesc& desc);
    void Destroy();

    struct RenderDesc
    {
        VkCommandBuffer commandBuffer;
        VkFramebuffer framebuffer;
    	const IRenderPass* renderPass;
    };
    void Run(const std::function<void(const Context::RenderDesc&)>& rendererCallback);

    void HintWindowResize();

    void GetScreenSize(int& width, int& height) const;

    [[nodiscard]] const IRenderPass* GetRenderPass() const;
    [[nodiscard]] const Swapchain* GetSwapchain() const;
    
    [[nodiscard]] const Device* GetDevice() const;
    [[nodiscard]] const DeviceQueue* GetGraphicsQueue() const;
    [[nodiscard]] std::optional<const DeviceQueue*> GetTransferQueue() const;
    [[nodiscard]] const DeviceQueue* GetActualTransferQueue() const;

    [[nodiscard]] VkCommandBuffer GetGraphicsCommandBuffer() const;
    [[nodiscard]] VkCommandBuffer GetTransferCommandBuffer() const;

    struct ShareInfo
    {
        std::vector<uint32_t> queueFamilyIndices;
        VkSharingMode sharingMode;
    };

    /**
     * Creating a resource that has to be available in both transfer and graphics queue could be problematic.
     * Returns proper queue family indices and sharing mode for transfer and graphics command buffers.
     */
    [[nodiscard]] ShareInfo GetTransferShareInfo() const;

    [[nodiscard]] VkInstance GetVkInstance() const;
    [[nodiscard]] GLFWwindow* GetGlfwWindow() const;

    void SetSwapchainImageCount(uint32_t count) const;
    [[nodiscard]] uint32_t GetSwapchainImageCount() const;
    [[nodiscard]] const Config* GetConfig() const;

private:

    void Update(const std::function<void(const Context::RenderDesc&)>& rendererCallback);
    void Render(const std::function<void(const Context::RenderDesc&)>& rendererCallback, uint32_t imageIndex);

    void InitializeWindow();
    void DestroyWindow();

    void InitializeVulkan();
    void DestroyVulkan();

    void CreateQueues();

    void CreateCommandPools();
    void DestroyCommandPools();

    void CreateCommandBuffers();
    void DestroyCommandBuffers();

    void ChooseMainDevice();
    void ResizeImmediately();

    void CreateFramebuffers();
    void DestroyFramebuffers();

    void CreateSyncObjects();
    void DestroySyncObjects();

    void InitializeImGui();
    void DestroyImGui();

    [[nodiscard]] std::vector<const char*> GetVulkanInstanceExtensions() const;
    [[nodiscard]] std::vector<const char*> GetVulkanValidationLayers() const;

    std::shared_ptr<Config> m_config{};
	IRenderPass* m_renderPass{};
    IComponentSystem* m_componentSystem{};

    /**
     * The extensions that the default context expects you to have.
     * But the lack of the extension won't crash the app.
     */
    std::vector<const char*> m_essentialDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
    };

    GLFWwindow* m_window{};
    VkInstance m_instance{};

    DeviceID m_mainDeviceID = -1;
    std::unique_ptr<Device> m_mainDevice{};

    std::unique_ptr<Surface> m_surface{};
    std::unique_ptr<Swapchain> m_swapchain{};

    bool m_mustResize{};
    std::vector<VkFramebuffer> m_framebuffers{};

    std::shared_ptr<DeviceQueue> m_graphicsQueue{};
    VkCommandPool m_graphicsCommandPool;
    VkCommandBuffer m_graphicsCommandBuffer;

    std::optional<std::shared_ptr<DeviceQueue>> m_transferQueue{};
    std::optional<VkCommandPool> m_transferCommandPool;
    VkCommandBuffer m_transferCommandBuffer{}; // Could be from the graphics command pool


    /* * *
     * Frame synchronization objects.
     */
    VkSemaphore m_imageAvailableSemaphore{};
    VkSemaphore m_renderFinishedSemaphore{};
    VkFence m_submitFrameFence{};


    VkDescriptorPool m_imguiDescriptorPool{};
};
