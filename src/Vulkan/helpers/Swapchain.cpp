#include "Swapchain.hpp"

#include "../App.hpp"

Swapchain::Swapchain(const App* app, const Surface* surface, const Device* device) {

    m_app = app;
    m_surface = surface;
    m_device = device;

    this->Initialize();
}

void Swapchain::Destroy() {

    for (const auto imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device->GetVkDevice(), imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_device->GetVkDevice(), m_swapChain, nullptr);

    m_swapChainImageViews.clear();
    m_swapChainImages.clear();
}

void Swapchain::Resize() {

    spdlog::info("[Swapchain] Resizing swapchain");
    this->Destroy();
    this->Initialize();
}

VkSwapchainKHR Swapchain::GetVkSwapchain() const {
    return m_swapChain;
}

VkImageView Swapchain::GetImage(int index) const {
    return m_swapChainImageViews[index];
}

uint32_t Swapchain::GetImageCount() const {
    return m_swapChainImageViews.size();
}

VkExtent2D Swapchain::GetExtent() const {
    return m_swapChainExtent;
}

VkFormat Swapchain::GetFormat() const {
    return m_swapChainImageFormat;
}

void Swapchain::Initialize() {
    const Device::SurfaceCapabilities capabilities = m_device->QuerySurfaceCapabilities(m_surface);

    const VkSurfaceFormatKHR surfaceFormat = this->ChooseSurfaceFormat(capabilities.formats);
    const VkPresentModeKHR presentMode = this->ChoosePresentMode(capabilities.presentModes);
    const VkExtent2D extent = this->ChooseSwapExtent(capabilities.surfaceCapabilities);

    spdlog::info("[Swapchain] Maximum supported number of swap chain images by the device: {}", capabilities.surfaceCapabilities.maxImageCount);
    if (m_app->GetSwapchainImageCount() > capabilities.surfaceCapabilities.maxImageCount) {
        spdlog::error("[Swapchain] The device doesn't support swap chain image count of {}", m_app->GetSwapchainImageCount());
        m_app->SetSwapchainImageCount(capabilities.surfaceCapabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface->GetVkSurface(),
        .minImageCount = m_app->GetSwapchainImageCount(),
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    // TODO: implement separation of presentation and graphics queue.
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    VkResult result = vkCreateSwapchainKHR(m_device->GetVkDevice(), &createInfo, nullptr, &m_swapChain);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Swapchain] Could not create swap chain: " + std::to_string(result));
    }

    m_swapChainImages.resize(m_app->GetSwapchainImageCount());

    uint32_t swapChainImageCount = m_swapChainImages.size();
    result = vkGetSwapchainImagesKHR(m_device->GetVkDevice(), m_swapChain, &swapChainImageCount, m_swapChainImages.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Swapchain] Could not retrieve swap chain images: " + std::to_string(result));
    }

    m_swapChainExtent = extent;
    m_swapChainImageFormat = surfaceFormat.format;

    this->CreateImageViews();
}

void Swapchain::CreateImageViews() {

    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t ind = 0; ind < m_swapChainImages.size(); ind++) {

        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapChainImages[ind],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_swapChainImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        const VkResult result = vkCreateImageView(m_device->GetVkDevice(), &createInfo, nullptr, &m_swapChainImageViews[ind]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[Swapchain] Could not create a swap chain image view: " + std::to_string(result));
        }
    }
}


VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {

    for (const auto& format : formats) {
        if (format.format == m_app->GetConfig()->vkPreferredSurfaceFormat && format.colorSpace == m_app->GetConfig()->vkPreferredSurfaceColorSpace) {
            return format;
        }
    }

    spdlog::warn("[Swapchain] Could not find the specified swapchain format. Using a default format");
    return formats[0];
}

VkPresentModeKHR Swapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const {

    if (std::ranges::find(presentModes, m_app->GetConfig()->vkPreferredPresentMode) != presentModes.end()) {
        return m_app->GetConfig()->vkPreferredPresentMode;
    }

    spdlog::warn("[Swapchain] Could not find the specified present mode. Using VK_PRESENT_MODE_FIFO_KHR");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {

    // Special value.
    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
        return capabilities.currentExtent;
    }

	spdlog::info("[Swapchain] Detected high DPI");

    int width, height;
	m_app->GetScreenSize(width, height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}