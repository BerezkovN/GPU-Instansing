#pragma once
#include "MainRenderer.hpp"

class InstancedRendererChunked : public MainRenderer
{

public:
	InstancedRendererChunked(const Context* context, MainComponentSystem* componentSystem);

	void Initialize(const std::string& vertexShader, const std::string& fragmentShader) override;
	void Destroy() override;

	void CreateInstanceBuffers();
	void UpdateInstanceBuffers();
	void DestroyInstanceBuffers();

	void Draw(VkCommandBuffer commandBuffer) override;
	void UpdateBuffers() override;

	MainRenderPipeline::VertexFormat GetVertexFormat() const override;

private:

	std::unique_ptr<GenericBuffer> m_instancedTranslationBuffer;
	std::unique_ptr<GenericBuffer> m_instancedRotationBuffer;
	std::unique_ptr<GenericBuffer> m_instancedSpriteBuffer;
};