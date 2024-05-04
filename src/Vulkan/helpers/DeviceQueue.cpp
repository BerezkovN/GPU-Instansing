#include "DeviceQueue.hpp"

DeviceQueue::DeviceQueue(uint32_t familyIndex, uint32_t queueIndex, float priority) {
	m_familyIndex = familyIndex;
	m_queueIndex = queueIndex;
	m_priority = priority;
}

void DeviceQueue::Initialize(VkQueue vkQueue) {
	m_vkQueue = vkQueue;
}

VkQueue DeviceQueue::GetVkQueue() const {
	return m_vkQueue;
}

uint32_t DeviceQueue::GetFamilyIndex() const {
	return m_familyIndex;
}

uint32_t DeviceQueue::GetQueueIndex() const {
	return m_queueIndex;
}

float DeviceQueue::GetPriority() const {
	return m_priority;
}
