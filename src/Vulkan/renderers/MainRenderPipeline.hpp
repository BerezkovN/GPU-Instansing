#pragma once

#include "../helpers/IRenderPipeline.hpp"
#include "../helpers/IRenderPass.hpp"
#include "../helpers/Device.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <array>

class MainRenderPipeline : public IRenderPipeline
{
public:

	struct Vertex
	{
		glm::vec3 position;
		int color;
	};

	
	struct UniformBufferObject
	{
		// Beware of alignment!
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	
	struct PipelineDescriptorSetInfo
	{
		VkBuffer buffer;
		VkDeviceSize offset;
		VkDeviceSize range;
		VkDescriptorType descriptorType;
	};

	static constexpr uint32_t DescriptorSetCount = 1;
	struct PipelineDescriptorSets
	{
		PipelineDescriptorSetInfo uniformBufferObject;
	};

	struct CreateDesc
	{
		const App* app;
		const Device* device;
		const IRenderPass* renderPass;
		VkShaderModule fragmentShader;
		VkShaderModule vertexShader;
		std::vector<PipelineDescriptorSets> descriptorSetWrites;
	};

	MainRenderPipeline(const MainRenderPipeline::CreateDesc& desc);
	void Destroy() override;

	void BindDescriptors(VkCommandBuffer buffer) override;
	
	VkPipeline GetVkPipeline() const override;
	VkPipelineLayout GetVkPipelineLayout() const override;

private:

	union PipelineVkDescriptorSets
	{
		struct {
			VkDescriptorSet uniformBufferObject;
		} named;

		std::array<VkDescriptorSet, DescriptorSetCount> indexed;
	};


	void CreateDescriptorSetLayout();
	void DestroyDescriptorSetLayout();


	void CreateDescriptorPool();
	void DestroyDescriptorPool();

	void CreateDescriptorSets(const std::vector<PipelineDescriptorSets>& descriptorSetWrites);


	const App* m_app;
	const Device* m_device;
	const IRenderPass* m_renderPass;

	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool m_descriptorPool;
	std::vector<PipelineVkDescriptorSets> m_descriptorSets;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

};
