#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <spirv_cross.hpp>

#include "Device.hpp"

class Shader
{
public:

	enum Type
	{
		Vertex,
		Fragment
	};

	Shader(const Device* device, const std::string& path, Shader::Type type);
	void Destroy();

	[[nodiscard]] const spirv_cross::Compiler* GetSpirvCompiler() const;

	[[nodiscard]] Shader::Type GetType() const;
	[[nodiscard]] VkShaderStageFlags GetVkType() const;

	[[nodiscard]] VkShaderModule GetVkShaderModule() const;
	
private:
	static std::vector<uint32_t> ReadSpirvData(const std::string& path);

	const Device* m_device;
	Shader::Type m_type;

	VkShaderModule m_shaderModule;
	std::unique_ptr<spirv_cross::Compiler> m_spirvCompiler;
};