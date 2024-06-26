#pragma once

#include <volk.h>

#include <cstdint>
#include <string>

class DeviceQueue {

public:
    enum Type {
        Graphics,
        Transfer,
        // TODO: Compute, etc.
    };

    static std::string QueueTypeToString(DeviceQueue::Type type);

    DeviceQueue(uint32_t familyIndex, uint32_t queueIndex, float priority);

    void Initialize(VkQueue vkQueue);

    [[nodiscard]] VkQueue GetVkQueue() const;
    [[nodiscard]] uint32_t GetFamilyIndex() const;
    [[nodiscard]] uint32_t GetQueueIndex() const;
    [[nodiscard]] float GetPriority() const;

private:
    uint32_t m_familyIndex;
    uint32_t m_queueIndex;
    float m_priority;

    VkQueue m_vkQueue;
};
