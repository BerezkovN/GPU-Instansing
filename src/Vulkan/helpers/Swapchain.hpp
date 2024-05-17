#pragma once

#include <volk.h>
#include <vector>


class App;
class Surface;
class Device;

class Swapchain {

public:

    Swapchain(const App* app, const Surface* surface, const Device* device);
    void Destroy();

    /**
     * Automatically resizes to the windows size. (Supports high DPI)
     */
    void Resize();

    [[nodiscard]] VkSwapchainKHR GetVkSwapchain() const;
    [[nodiscard]] VkImageView GetImage(int index) const;
    [[nodiscard]] uint32_t GetImageCount() const;

    [[nodiscard]] VkExtent2D GetExtent() const;
    [[nodiscard]] VkFormat GetFormat() const;

private:

    void Initialize();
    void CreateImageViews();

    [[nodiscard]] VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    [[nodiscard]] VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
    [[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    const App* m_app;
    const Surface* m_surface;
    const Device* m_device;

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;

    VkFormat m_swapChainImageFormat{};
    VkExtent2D m_swapChainExtent{};
};
