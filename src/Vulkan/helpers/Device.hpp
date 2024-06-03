#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <volk.h>

#include "DeviceQueue.hpp"
#include "DeviceMemory.hpp"

class Context;
class Surface;

typedef size_t DeviceID;

class Device {

	/**
	 * Device can only be created through factory methods.
	 */
	Device(const Context* context, VkPhysicalDevice physicalDevice);

public:

    struct SurfaceCapabilities
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static DeviceID CalculateID(const VkPhysicalDeviceProperties& deviceProperties);

    // Factory
    static std::unique_ptr<Device> FindDevice(const Context* context, DeviceID deviceID);
	void Destroy();

	void WaitIdle() const;

    [[nodiscard]] bool DoesSupportRendering(const Surface* surface) const;
    [[nodiscard]] bool DoesSupportExtension(const std::string& extensionName) const;
    [[nodiscard]] bool IsExtensionEnabled(const std::string& extensionName) const;

	/**
	 * \param surface You must pass a surface if the queue type is Graphics.
	 * \return Handle to not yet initialized queue.
	 */
    std::optional<std::shared_ptr<DeviceQueue>> AddQueue(DeviceQueue::Type queueType, float priority, const Surface* surface = nullptr);

    void Initialize();

    [[nodiscard]] DeviceMemory* GetDeviceMemory() const;
    [[nodiscard]] SurfaceCapabilities QuerySurfaceCapabilities(const Surface* surface) const;
    [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const;
    [[nodiscard]] VkPhysicalDeviceProperties GetVkPhysicalDeviceProperties() const;
    [[nodiscard]] VkDevice GetVkDevice() const;

private:

    uint32_t FindGraphicsQueueFamilyIndex(const Surface* surface) const;
	uint32_t FindTransferQueueFamilyIndex() const;

    const Context* m_context{};

    std::vector<const char*> m_enabledExtensions;

    VkPhysicalDevice m_physicalDevice{};
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkPhysicalDeviceFeatures m_physicalDeviceFeatures;

    std::vector<VkExtensionProperties> m_supportedExtensions;

    std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
    typedef std::vector<std::shared_ptr<DeviceQueue>> QueueFamily;
	std::vector<QueueFamily> m_queueFamilies;

    std::unique_ptr<DeviceMemory> m_deviceMemory;

    VkDevice m_logicalDevice{};
};
