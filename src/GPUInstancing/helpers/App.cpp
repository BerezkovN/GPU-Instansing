#include "App.hpp"

#include <algorithm>
#include <limits>

#include "pch.hpp"
#include "VkHelper.hpp"

void App::Start() {

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow_ = glfwCreateWindow(800, 600, "GPUInstancing", nullptr, nullptr);

    this->InitializeVulkan();
    this->CreateSurface();
    this->PickGPU();
    this->CreateLogicalDevice();

    //while (!glfwWindowShouldClose(glfwWindow_)) {
    //	glfwPollEvents();

    //	this->Update();
    //}

    this->DestroyVulkan();

    glfwDestroyWindow(glfwWindow_);
    glfwTerminate();
 
}


void App::InitializeVulkan() {

    VkApplicationInfo appInfo
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "GPUInstancing",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "None",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t extensionCount;
    uint32_t layerCount;

    const auto extensions = this->GetVulkanExtensions(extensionCount);
    const auto layers = this->GetVulkanValidationLayers(layerCount);

    const VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layers,
    	.enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensions,
    };

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance_);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create Vulkan instance");
    }
}

void App::DestroyVulkan() const {
    vkDestroyDevice(vkLogicalDevice_, nullptr);
    vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
    vkDestroyInstance(vkInstance_, nullptr);
}

void App::Update() {

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;
	
}

void App::CreateSurface() {

	const VkResult result = glfwCreateWindowSurface(vkInstance_, glfwWindow_, nullptr, &vkSurface_);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create Vulkan surface");
    }
}

void App::PickGPU() {
    uint32_t deviceCount;

    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("[App] No GPUs were found");
    }

    std::vector<VkPhysicalDevice> gpus(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, gpus.data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto& gpu : gpus) {
	    if (this->IsGPUSupported(gpu)) {
            physicalDevice = gpu;
            break;
	    }
    }


    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("[App] No suitable GPUs were found");
    }

    vkPhysicalDevice_ = physicalDevice;
}

bool App::IsGPUSupported(const VkPhysicalDevice physicalDevice) {

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    std::cout << "[App] " << vk_to_string(deviceProperties.deviceType) << " : " << deviceProperties.deviceName << '\n';

    this->FindFamilyQueues(physicalDevice);

    const bool supportsExtensions = this->DoesSupportDeviceExtensions(physicalDevice);

    bool supportsSwapChain = false;
    if (supportsExtensions) {
	    const SwapChainSupportInfo swapInfo = this->QuerySwapChainSupport(physicalDevice);
        supportsSwapChain = !swapInfo.presentModes.empty() && !swapInfo.formats.empty();
    }

    return
		deviceProperties.deviceType == vkRequiredDeviceType_ &&
        supportsExtensions &&
        supportsSwapChain &&
        graphicsQueueIndex_.has_value();
}

void App::FindFamilyQueues(const VkPhysicalDevice physicalDevice) {

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t index{};

    std::cout << "[App] Supported queue families:\n";
    for (const auto& queueFamily : queueFamilyProperties) {

        std::cout << "\tQueue count: " << queueFamily.queueCount << '\n';
        std::cout << "\tQueue flags: " << vk_to_string(queueFamily.queueFlags) << '\n';

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

            VkBool32 isSurfaceSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, vkSurface_, &isSurfaceSupported);

            if (!isSurfaceSupported) {
                continue;
            }

            graphicsQueueIndex_ = index;
        }

        index++;
    }
}

bool App::DoesSupportDeviceExtensions(const VkPhysicalDevice physicalDevice) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProperties.data());

    return does_contain_names<VkExtensionProperties>(vkDeviceExtensions_, extensionProperties, [](auto& property) { return property.extensionName; });
}

App::SwapChainSupportInfo App::QuerySwapChainSupport(const VkPhysicalDevice physicalDevice) const {
    SwapChainSupportInfo details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkSurface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface_, &formatCount, nullptr);
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface_, &formatCount, details.formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface_, &presentModeCount, nullptr);
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface_, &presentModeCount, details.presentModes.data());

    return details;
}

VkSurfaceFormatKHR App::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {


	for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
	}

    std::cout << "[App] Could not find the specified swapchain format. Using a default format\n";

    return formats[0];
}

VkPresentModeKHR App::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const {

    if (std::ranges::find(presentModes, VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D App::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {

    // Special value.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        std::cout << "[App] Not High DPI\n";
        return capabilities.currentExtent;
    }

    std::cout << "[App] High DPI\n";

    int width, height;
    glfwGetFramebufferSize(glfwWindow_, &width, &height);

    VkExtent2D actualExtent = {
	    static_cast<uint32_t>(width),
	    static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void App::CreateLogicalDevice() {

    VkDeviceQueueCreateInfo graphicsQueueInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueIndex_.value(),
        .queueCount = 1,
        .pQueuePriorities = &graphicsQueuePriority_
    };

    VkPhysicalDeviceFeatures deviceFeatures{};

    // Used for compatibility with older devices.
    uint32_t layerCount;
    const auto layers = this->GetVulkanValidationLayers(layerCount);

    const VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &graphicsQueueInfo,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = static_cast<uint32_t>(vkDeviceExtensions_.size()),
        .ppEnabledExtensionNames = vkDeviceExtensions_.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    const VkResult result = vkCreateDevice(vkPhysicalDevice_, &deviceCreateInfo, nullptr, &vkLogicalDevice_);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create logical device");
    }

    vkGetDeviceQueue(vkLogicalDevice_, graphicsQueueIndex_.value(), 0, &graphicsQueue_);
}

void App::CreateSwapChain() {

    const SwapChainSupportInfo swapChainInfo = this->QuerySwapChainSupport(vkPhysicalDevice_);

    const VkSurfaceFormatKHR surfaceFormat = this->ChooseSurfaceFormat(swapChainInfo.formats);
    const VkPresentModeKHR presentMode = this->ChoosePresentMode(swapChainInfo.presentModes);
    const VkExtent2D extent = this->ChooseSwapExtent(swapChainInfo.capabilities);

    std::cout << "[App] Maximum supported number of swap chain images by the device: " << swapChainInfo.capabilities.maxImageCount << '\n';
    if (swapChainImageCount_ > swapChainInfo.capabilities.maxImageCount) {
        std::cout << "[App] The device doesn't support swap chain image count of " << swapChainImageCount_ << '\n';
        swapChainImageCount_ = swapChainInfo.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkSurface_,
        .minImageCount = swapChainImageCount_,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
}

const char* const* App::GetVulkanValidationLayers(uint32_t& layerCount) const {

#if !NDEBUG
    layerCount = static_cast<uint32_t>(vkValidationLayers_.size());

    uint32_t supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    does_contain_names<VkLayerProperties>(
        vkValidationLayers_, supportedLayers, 
        [](auto& layerProperty) { return layerProperty.layerName; });

    return vkValidationLayers_.data();
#else
    layerCount = 0;
    return nullptr;
#endif
}

// ReSharper disable once CppMemberFunctionMayBeStatic
const char* const* App::GetVulkanExtensions(uint32_t& extensionCount) const {
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    // Add extra extensions if needed.

    return glfwExtensions;
}
