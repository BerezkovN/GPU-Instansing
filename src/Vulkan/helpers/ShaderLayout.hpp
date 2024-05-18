#pragma once

#include <vector>

#include "Shader.hpp"
#include "Device.hpp"

#include "./buffers/GenericBuffer.hpp"
#include "./textures/Sampler.hpp"

class ShaderLayout
{
public:

	struct DescriptorID
	{
		uint32_t set;
		uint32_t binding;

		/**
		 * A binding can have multiple descriptors.
		 */
		uint32_t index;
	};

	ShaderLayout(const Device* device, const Shader* vertexShader, const Shader* fragmentShader);
	void Destroy();

	void AttachBuffer(const DescriptorID& id, const GenericBuffer* buffer, VkDeviceSize offset, VkDeviceSize range);
	void AttackSampler(const DescriptorID& id, const Sampler* sampler);

	void AttachBuffer(const std::string& name, const GenericBuffer* buffer, VkDeviceSize offset, VkDeviceSize range);
	void AttackSampler(const std::string& name, const Sampler* sampler);

	void BindDescriptors(VkCommandBuffer buffer) const;

	[[nodiscard]] DescriptorID GetDescriptorID(const std::string& name);

	[[nodiscard]] VkPipelineLayout GetVkPipelineLayout() const;
	[[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetVkShaderStages() const;
	
private:

	void ParseShader(const Shader* shader, std::vector<VkDescriptorPoolSize>& poolSizes);
	void ParseResourceType(VkDescriptorType type, const Shader* shader, std::vector<VkDescriptorPoolSize>& poolSizes);

	void CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes);
	void DestroyDescriptorPool();

	void CreateDescriptorSetLayout();
	void DestroyDescriptorSetLayout();

	void AllocateDescriptorSets();

	const Device* m_device;
	const Shader* m_fragmentShader;
	const Shader* m_vertexShader;


	struct BindingInfo
	{
		uint32_t bindingIndex;
		uint32_t setIndex;
		std::string name;
		VkShaderStageFlags visibility;
		VkDescriptorType type;
	};
	typedef std::vector<std::optional<BindingInfo>> SetInfo;
	std::vector<std::optional<SetInfo>> m_descriptorSetsInfo;

	std::unordered_map<std::string, DescriptorID> m_descriptorIdMap;

	VkDescriptorPool m_descriptorPool;

	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkDescriptorSet> m_descriptorSets;

	VkPipelineLayout m_pipelineLayout;

	static spirv_cross::SmallVector<spirv_cross::Resource> GetResourceFromType(const spirv_cross::ShaderResources& resources, VkDescriptorType type);

};
