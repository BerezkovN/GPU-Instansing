#include "App.hpp"

#include "pch.hpp"
#include "VkHelper.hpp"

void App::Start() {

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow_ = glfwCreateWindow(800, 600, "GPUInstancing", nullptr, nullptr);

    this->InitializeVulkan();
    this->PickGPU();

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
    vkDestroyInstance(vkInstance_, nullptr);
}

void App::Update() {

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;
	
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

bool App::IsGPUSupported(const VkPhysicalDevice physicalDevice) const {

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    std::cout << "[App] " << vk_to_string(deviceProperties.deviceType) << " : " << deviceProperties.deviceName << '\n';

    const auto familyQueueIndex = this->FindFamilyQueue(physicalDevice);

    return deviceProperties.deviceType == vkRequiredDeviceType_ && familyQueueIndex.has_value();
}

std::optional<uint32_t> App::FindFamilyQueue(const VkPhysicalDevice physicalDevice) const {

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    std::optional<uint32_t> result{};
    uint32_t index{};

    std::cout << "[App] Supported queue families:\n";
    for (const auto& queueFamily : queueFamilyProperties) {

        std::cout << "\tQueue count: " << queueFamily.queueCount << '\n';
        std::cout << "\tQueue flags: " << vk_to_string(queueFamily.queueFlags) << '\n';

        if (queueFamily.queueFlags & vkRequiredQueueFlags_) {
            result = index;
        }

        index++;
    }

    return result;
}


const char* const* App::GetVulkanValidationLayers(uint32_t& layerCount) const {

#if !NDEBUG
    layerCount = static_cast<uint32_t>(vkValidationLayers_.size());

    uint32_t supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    for (const auto requestedLayer : vkValidationLayers_) {

        bool wasLayerFound = false;
        for (const auto& supportedLayer : supportedLayers) {

            if (strcmp(requestedLayer, supportedLayer.layerName) != 0) {
                wasLayerFound = true;
                break;
            }
        }

        if (!wasLayerFound) {
            throw std::runtime_error("[App] System does not support the following validation layer: " + std::string(requestedLayer));
        }
    }

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
