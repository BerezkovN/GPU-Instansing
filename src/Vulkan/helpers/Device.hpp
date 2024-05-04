#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Surface.hpp"
#include "DeviceQueue.hpp"

typedef size_t DeviceID;

class App;

class Device {

	/**
	 * Device can only be created through factory methods.
	 */
	Device(const App* app, VkPhysicalDevice physicalDevice);

public:

    struct SurfaceCapabilities
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static DeviceID CalculateID(const VkPhysicalDeviceProperties& deviceProperties);

    // Factory
    static std::unique_ptr<Device> FindDevice(const App* app, DeviceID deviceID);

    void Destroy();

    void WaitIdle();
    bool PrepareForRendering(const Surface* surface);
    bool DoesSupportExtension(const std::string& extensionName);

	/**
	 * \param surface You must pass a surface if the queue type is Graphics.
	 * \return Handle to not yet initialized queue.
	 */
    std::shared_ptr<DeviceQueue> AddQueue(DeviceQueue::Type queueType, float priority, const Surface* surface = nullptr);

    void Initialize();

    [[nodiscard]] const SurfaceCapabilities* GetSurfaceCapabilities() const;
    [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const;
    [[nodiscard]] VkDevice GetVkDevice() const;

private:

    const App* m_app;

    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkPhysicalDeviceFeatures m_physicalDeviceFeatures;

    std::vector<VkExtensionProperties> m_supportedExtensions;

    std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
    std::vector<int> m_queueFamilyCounters;

    std::vector<std::shared_ptr<DeviceQueue>> m_queues;

    std::unique_ptr<SurfaceCapabilities> m_surfaceCapabilities;

    VkDevice m_logicalDevice;
};
