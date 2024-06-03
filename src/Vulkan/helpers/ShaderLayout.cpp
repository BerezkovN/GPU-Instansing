#include "ShaderLayout.hpp"

#include "../pch.hpp"

#include "./buffers/GenericBuffer.hpp"
#include "./textures/Sampler.hpp"
#include "Shader.hpp"
#include "Device.hpp"

ShaderLayout::ShaderLayout(const Device* device, const Shader* vertexShader, const Shader* fragmentShader) {

    m_device = device;
	m_fragmentShader = fragmentShader;
	m_vertexShader = vertexShader;

    std::vector<VkDescriptorPoolSize> poolSizes{};
    this->ParseShader(m_vertexShader, poolSizes);
    this->ParseShader(m_fragmentShader, poolSizes);

    this->CreateDescriptorPool(poolSizes);
	this->CreateDescriptorSetLayout();
    this->AllocateDescriptorSets();

	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size()),
		.pSetLayouts = m_descriptorSetLayouts.data(),
        .pushConstantRangeCount = m_pushConstantRange.has_value() ? 1u : 0u,
        .pPushConstantRanges = m_pushConstantRange.has_value() ? &m_pushConstantRange.value() : nullptr
	};

	const VkResult result = vkCreatePipelineLayout(m_device->GetVkDevice(), &pipelineLayoutCreateInfo, nullptr,
	                                         &m_pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[ShaderLayout] Could not create pipeline layout: " + std::to_string(result));
	}
}

void ShaderLayout::Destroy() {

    this->DestroyDescriptorPool();
    this->DestroyDescriptorSetLayout();
    vkDestroyPipelineLayout(m_device->GetVkDevice(), m_pipelineLayout, nullptr);

}

void ShaderLayout::AttachBuffer(const DescriptorID& id, const GenericBuffer* buffer, VkDeviceSize offset, VkDeviceSize range) {
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = buffer->GetVkBuffer(),
        .offset = offset,
        .range = range
    };

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSets[id.set],
        .dstBinding = id.binding,
        .dstArrayElement = id.index,
        .descriptorCount = 1,           // TODO: Support array bindings.
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo
    };

    vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &write, 0, nullptr);
}

void ShaderLayout::AttackSampler(const DescriptorID& id, const Sampler* sampler) {

    VkDescriptorImageInfo imageInfo = {
        .sampler = sampler->GetVkSampler(),
        .imageView = sampler->GetVkImageView(),
        .imageLayout = sampler->GetVkImageLayout()
    };

    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSets[id.set],
        .dstBinding = id.binding,
        .dstArrayElement = id.index,
        .descriptorCount = 1,           // TODO: Support array bindings.
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &write, 0, nullptr);
}

void ShaderLayout::AttachBuffer(const std::string& name, const GenericBuffer* buffer, VkDeviceSize offset, VkDeviceSize range) {
    this->AttachBuffer(this->GetDescriptorID(name), buffer, offset, range);
}

void ShaderLayout::AttackSampler(const std::string& name, const Sampler* sampler) {
    this->AttackSampler(this->GetDescriptorID(name), sampler);
}

VkPipelineLayout ShaderLayout::GetVkPipelineLayout() const {
    return m_pipelineLayout;
}

void ShaderLayout::BindDescriptors(VkCommandBuffer buffer) const {

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 
        0, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data(),
        0, nullptr);
}

ShaderLayout::DescriptorID ShaderLayout::GetDescriptorID(const std::string& name) {
    if (!m_descriptorIdMap.contains(name)) {
        throw std::runtime_error("[ShaderLayout] Did not find a descriptor with such name: " + name);
    }

    return m_descriptorIdMap[name];
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderLayout::GetVkShaderStages() const {


    // TODO: Use pUseSpecializationInfo for GPU Instancing variants.
    const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = m_vertexShader->GetVkShaderModule(),
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = m_fragmentShader->GetVkShaderModule(),
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

	return { vertShaderStageInfo, fragShaderStageInfo };
}

void ShaderLayout::ParseShader(const Shader* shader, std::vector<VkDescriptorPoolSize>& poolSizes) {

    this->ParseResourceType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shader, poolSizes);
    this->ParseResourceType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shader, poolSizes);

    const spirv_cross::Compiler* compiler = shader->GetSpirvCompiler();
    const spirv_cross::ShaderResources& shaderResources = compiler->get_shader_resources();

    if (shaderResources.push_constant_buffers.empty()) {
        return;
    }

    // There could be only one push constant.
    const spirv_cross::Resource pushConstant = shaderResources.push_constant_buffers[0];
    const uint32_t pushConstantSize = static_cast<uint32_t>(compiler->get_declared_struct_size(compiler->get_type(pushConstant.base_type_id)));

    m_pushConstantRange = VkPushConstantRange{
        .stageFlags = shader->GetVkType(),
        .offset = 0,
        .size = pushConstantSize,
    };
}

void ShaderLayout::ParseResourceType(VkDescriptorType type, const Shader* shader, std::vector<VkDescriptorPoolSize>& poolSizes) {

    const spirv_cross::Compiler* compiler = shader->GetSpirvCompiler();
    const spirv_cross::ShaderResources& shaderResources = compiler->get_shader_resources();

    const auto& resources = ShaderLayout::GetResourceFromType(shaderResources, type);

    // We do not need to allocate more descriptors when the same resource appears in another shader.
    uint32_t descriptorCount = 0;

    for (const spirv_cross::Resource& resource : resources) {

        const uint32_t set = compiler->get_decoration(resource.id, spv::DecorationDescriptorSet);
        const uint32_t binding = compiler->get_decoration(resource.id, spv::DecorationBinding);

        if (set >= m_descriptorSetsInfo.size()) {
            m_descriptorSetsInfo.resize(set + 1);
        }

        if (!m_descriptorSetsInfo[set].has_value()) {
            m_descriptorSetsInfo[set] = SetInfo{};
        }

        SetInfo& setInfo = m_descriptorSetsInfo[set].value();

        if (binding >= setInfo.size()) {
            setInfo.resize(binding + 1);
        }

        if (!setInfo[binding].has_value()) {
            setInfo[binding] = BindingInfo{
                .bindingIndex = binding,
                .setIndex = set,
                .name = resource.name,
                .visibility = shader->GetVkType(),
                .type = type
            };
            descriptorCount += 1;
        }
        else {
            setInfo[binding]->visibility |= shader->GetVkType();
        }
    }

    if (descriptorCount == 0) {
        return;
    }

    poolSizes.push_back(VkDescriptorPoolSize{
		.type = type,
		.descriptorCount = descriptorCount
    });
}


void ShaderLayout::CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes) {

    const uint32_t maxSets = std::accumulate(poolSizes.begin(), poolSizes.end(), 0, [](int sum, const VkDescriptorPoolSize& size)
    {
	    return sum + size.descriptorCount;
    });

    const VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    const VkResult result = vkCreateDescriptorPool(m_device->GetVkDevice(), &poolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[ShaderLayout] Could not create descriptor pool");
    }
}

void ShaderLayout::DestroyDescriptorPool() {

    // Vulkan automatically destructs descriptor set objects when destroying the pool.
    m_descriptorSets.clear();

    vkDestroyDescriptorPool(m_device->GetVkDevice(), m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;
}


void ShaderLayout::CreateDescriptorSetLayout() {

    m_descriptorSetLayouts.resize(m_descriptorSetsInfo.size());

    for (size_t setInd = 0; setInd < m_descriptorSetsInfo.size(); setInd++) {

        const auto& setInfo = m_descriptorSetsInfo[setInd];

        if (!setInfo.has_value()) {
            throw std::runtime_error("[ShaderLayout] Descriptor set indices must be continuous!!!");
        }

        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        for (const auto& bindingInfo : setInfo.value()) {

            if (!bindingInfo.has_value()) {
                throw std::runtime_error("[ShaderLayout] Descriptor binding indices must be continuous!!!");
            }

            bindings.push_back({
                .binding = bindingInfo->bindingIndex,
                .descriptorType = bindingInfo->type,
                .descriptorCount = 1, // This is cursed. Perhaps should be implemented later https://stackoverflow.com/questions/65772848/what-does-vkdescriptorsetlayoutbindingdescriptorcount-specify
                .stageFlags = bindingInfo->visibility,
                .pImmutableSamplers = nullptr // This is weird. Should we consider using this?
            });
        }

        VkDescriptorSetLayoutCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = 0,                                                         // TODO: Learn more about this
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };


        const VkResult result = vkCreateDescriptorSetLayout(m_device->GetVkDevice(), &createInfo, nullptr, &m_descriptorSetLayouts[setInd]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[ShaderLayout] Could not create descriptor set layout");
        }
    }
}

void ShaderLayout::DestroyDescriptorSetLayout() {
    for (const auto& layout : m_descriptorSetLayouts) {
		vkDestroyDescriptorSetLayout(m_device->GetVkDevice(), layout, nullptr);
    }
    m_descriptorSetLayouts.clear();
}

void ShaderLayout::AllocateDescriptorSets() {

    m_descriptorSets.resize(m_descriptorSetsInfo.size());

    const VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(m_descriptorSets.size()),
        .pSetLayouts = m_descriptorSetLayouts.data()
    };

    const VkResult result = vkAllocateDescriptorSets(m_device->GetVkDevice(), &allocateInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[ShaderLayout] Could not allocate descriptor sets: " + std::to_string(result));
    }

    for (const auto& setInfo : m_descriptorSetsInfo) {

        if (!setInfo.has_value()) {
            throw std::runtime_error("[ShaderLayout] Something went wrong with set parsing");
        }

        for (const auto& bindingInfo : setInfo.value()) {

            if (!bindingInfo.has_value()) {
                throw std::runtime_error("[ShaderLayout] Something went wrong with binding parsing");
            }

            m_descriptorIdMap[bindingInfo->name] = DescriptorID{
                .set = bindingInfo->setIndex,
                .binding = bindingInfo->bindingIndex,
                .index = 0
            };
        }
    }
}


spirv_cross::SmallVector<spirv_cross::Resource>
ShaderLayout::GetResourceFromType(const spirv_cross::ShaderResources& resources, VkDescriptorType type) {
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return resources.uniform_buffers;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return resources.sampled_images;
    default:
        throw std::runtime_error("[ShaderLayout] Descriptor type is not implemented");
    }
}