#include "Device.hpp"

#include "../pch.hpp"

#include "Context.hpp"
#include "Surface.hpp"
#include "DeviceMemory.hpp"

#include "VkHelper.hpp"


DeviceID Device::CalculateID(const VkPhysicalDeviceProperties& deviceProperties) {

    DeviceID result = 17;

    std::string name = deviceProperties.deviceName;
    result = 31 * result + std::hash<std::string>{}(name);

    return result;
}


Device::Device(const Context* context, VkPhysicalDevice physicalDevice) {

    static int deviceCount = 0;
    deviceCount += 1;

    if (deviceCount >= 2) {
        // TODO: Implement multiple device support.
        throw std::runtime_error("[Device] The current implementation does not support multiple devices.");
    }

    m_context = context;
    m_physicalDevice = physicalDevice;

    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_physicalDeviceFeatures);

    spdlog::info("[Device] Push constants size: {}", m_physicalDeviceProperties.limits.maxMemoryAllocationCount);

    // Extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    m_supportedExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, m_supportedExtensions.data());

    // Queues
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    m_queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, m_queueFamilyProperties.data());

    m_queueFamilies.resize(queueFamilyCount);

    spdlog::info("[Device] Supported queue families:");
    for (const auto& queueFamily : m_queueFamilyProperties) {

        auto queueFlagName = VkHelper::QueueFlagsToString(queueFamily.queueFlags);

        spdlog::info("[Device] \tQueue count: {}", queueFamily.queueCount);
        spdlog::info("[Device] \tQueue flags: {}", queueFlagName);
    }
    spdlog::info("");

    // Initialized later.
    m_logicalDevice = VK_NULL_HANDLE;
}

std::unique_ptr<Device> Device::FindDevice(const Context* context, DeviceID deviceID) {

    uint32_t deviceCount;

    vkEnumeratePhysicalDevices(context->GetVkInstance(), &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("[Device] No GPUs were found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context->GetVkInstance(), &deviceCount, devices.data());

    size_t chosenDeviceIndex = -1;

    for (size_t ind = 0; ind < devices.size(); ind++) {

        const auto& device = devices[ind];

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);


        if (deviceID == Device::CalculateID(deviceProperties)) {
            chosenDeviceIndex = ind;
        }
    }

    if (chosenDeviceIndex == -1) {
        spdlog::error("[Device] Could not find a device with id of {}. Choosing device at random.", deviceID);
        chosenDeviceIndex = 0;
    }

    // Hacky way to create unique pointer with private constructor.
    return std::unique_ptr<Device>(new Device(context, devices[chosenDeviceIndex]));
}


void Device::Destroy() {
    vkDestroyDevice(m_logicalDevice, nullptr);
    m_logicalDevice = VK_NULL_HANDLE;

    m_queueFamilyProperties.clear();
    m_queueFamilies.clear();
    m_queueFamilies.clear();
}

void Device::WaitIdle() const {
    vkDeviceWaitIdle(m_logicalDevice);
}

bool Device::DoesSupportRendering(const Surface* surface) const {

    if (!this->DoesSupportExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return false;
    }

    const auto capabilities = this->QuerySurfaceCapabilities(surface);
    if (capabilities.formats.empty() || capabilities.presentModes.empty()) {
        return false;
    }

    return true;
}

bool Device::DoesSupportExtension(const std::string& extensionName) const {

    const auto condition = [extensionName](const VkExtensionProperties& property) { return extensionName == property.extensionName; };
	const bool support = std::ranges::find_if(m_supportedExtensions, condition) != m_supportedExtensions.end();

    if (!support) {
        spdlog::error("[Device] Device doesn't support the extension: {}", extensionName);
    }

    return support;
}

bool Device::IsExtensionEnabled(const std::string& extensionName) const {

    auto condition = [&](const char* enabledExtension) { return extensionName == enabledExtension; };
    return std::ranges::find_if(m_enabledExtensions, condition) != m_enabledExtensions.end();
}

std::optional<std::shared_ptr<DeviceQueue>> Device::AddQueue(const DeviceQueue::Type queueType, float priority, const Surface* surface) {

    uint32_t familyIndex{};

    switch (queueType) {
    case DeviceQueue::Type::Graphics:
        familyIndex = this->FindGraphicsQueueFamilyIndex(surface);
        break;
    case DeviceQueue::Type::Transfer:
        familyIndex = this->FindTransferQueueFamilyIndex();
        break;
    default:
        throw std::runtime_error("[Device] Unsupported queue type");
    }

    if (m_queueFamilies[familyIndex].size() >= m_queueFamilyProperties[familyIndex].queueCount) {
        spdlog::warn("[Device] Not enough queues for the {} in the queue family {}", DeviceQueue::QueueTypeToString(queueType), familyIndex);
        return std::nullopt;
    }

    const auto result = std::make_shared<DeviceQueue>(familyIndex, m_queueFamilies[familyIndex].size(), priority);
    m_queueFamilies[familyIndex].push_back(result);

    return std::make_optional(result);
}

void Device::Initialize() {

    std::vector<VkDeviceQueueCreateInfo> graphicsQueueInfos;
    std::vector<std::vector<float>> priorities(m_queueFamilies.size());

    for (size_t familyInd = 0; familyInd < m_queueFamilies.size(); familyInd++) {

        if (m_queueFamilies[familyInd].empty()) {
            continue;
        }

        priorities[familyInd].reserve(m_queueFamilies[familyInd].size());

        for (const auto& queue : m_queueFamilies[familyInd]) {
            priorities[familyInd].push_back(queue->GetPriority());
        }

        VkDeviceQueueCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = static_cast<uint32_t>(familyInd),
                .queueCount = static_cast<uint32_t>(priorities[familyInd].size()),
                .pQueuePriorities = priorities[familyInd].data()
        };
        graphicsQueueInfos.push_back(info);
    }


    // I'm not planning on modifying the config's device extension field. It makes it quite unpredictable.
    m_enabledExtensions = m_context->GetConfig()->vkDeviceExtensions;

    for (const auto essentialExtension: m_context->m_essentialDeviceExtensions) {

        auto condition = [&](const char* enabledExtension)
        {
	        return std::strcmp(enabledExtension, essentialExtension) == 0;
        };

        // Do not include the essential extension if the extension was already added to the config.
    	if (std::ranges::find_if(m_enabledExtensions, condition) == m_enabledExtensions.end()) {
            m_enabledExtensions.push_back(essentialExtension);
    	}
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    const VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = static_cast<uint32_t>(graphicsQueueInfos.size()),
            .pQueueCreateInfos = graphicsQueueInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size()),
            .ppEnabledExtensionNames = m_enabledExtensions.data(),
            .pEnabledFeatures = &deviceFeatures
    };

    const VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice);

    // https://gpuopen.com/learn/reducing-vulkan-api-call-overhead/
    volkLoadDevice(m_logicalDevice);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Device] Could not create logical device: " + std::to_string(result));
    }

    for (const auto& queueFamily: m_queueFamilies) {

        for (const auto& queue : queueFamily) {

            VkQueue vkQueue;
            vkGetDeviceQueue(m_logicalDevice, queue->GetFamilyIndex(), queue->GetQueueIndex(), &vkQueue);

            queue->Initialize(vkQueue);
        }
    }

    m_memory = std::make_unique<DeviceMemory>(this);
}

Device::SurfaceCapabilities Device::QuerySurfaceCapabilities(const Surface* surface) const {
    SurfaceCapabilities capabilities;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface->GetVkSurface(), &capabilities.surfaceCapabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface->GetVkSurface(), &formatCount, nullptr);
    capabilities.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface->GetVkSurface(), &formatCount, capabilities.formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface->GetVkSurface(), &presentModeCount, nullptr);
    capabilities.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface->GetVkSurface(), &presentModeCount, capabilities.presentModes.data());

    return capabilities;
}

VkPhysicalDevice Device::GetVkPhysicalDevice() const {
    return m_physicalDevice;
}

VkDevice Device::GetVkDevice() const {
    return m_logicalDevice;
}

uint32_t Device::FindGraphicsQueueFamilyIndex(const Surface* surface) const {
	uint32_t index{};
    const auto& graphicsQueueIt = std::ranges::find_if(m_queueFamilyProperties, [&](const VkQueueFamilyProperties& queueFamily)
    {
        if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            return false;
        }

        VkBool32 isSurfaceSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, index, surface->GetVkSurface(), &isSurfaceSupported);

        index++;
        return static_cast<bool>(isSurfaceSupported);
    });

    return static_cast<uint32_t>(std::distance(m_queueFamilyProperties.begin(), graphicsQueueIt));
}

uint32_t Device::FindTransferQueueFamilyIndex() const {

    // Ideally, we would want graphics and transfer queue to be on different queue families.
    const auto& idealTransferQueueIt = std::ranges::find_if(m_queueFamilyProperties, [=](const VkQueueFamilyProperties& queueFamily)
    {
        return (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    });

    if (idealTransferQueueIt != m_queueFamilyProperties.end()) {
        return static_cast<uint32_t>(std::distance(m_queueFamilyProperties.begin(), idealTransferQueueIt));
    }

    const auto& transferQueueIt = std::ranges::find_if(m_queueFamilyProperties, [=](const VkQueueFamilyProperties& queueFamily)
    {
        return queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;
    });

    return static_cast<uint32_t>(std::distance(m_queueFamilyProperties.begin(), transferQueueIt));
}

