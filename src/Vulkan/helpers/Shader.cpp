#include "Shader.hpp"

#include <fstream>
#include <filesystem>

#include <spirv_cross.hpp>

Shader::Shader(const Device* device, const std::string& path, Shader::Type type) {

	m_device = device;
	m_type = type;

	std::vector<uint32_t> spirvData = Shader::ReadSpirvData(path);
	m_spirvCompiler = std::make_unique<spirv_cross::Compiler>(spirvData);
	
	const VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirvData.size() * 4,
		.pCode = spirvData.data()
	};

	const VkResult result = vkCreateShaderModule(device->GetVkDevice(), &createInfo, nullptr, &m_shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Shader] Could not create shader module: " + std::to_string(result));
	}
}

void Shader::Destroy() {
	vkDestroyShaderModule(m_device->GetVkDevice(), m_shaderModule, nullptr);
	m_shaderModule = VK_NULL_HANDLE;
}

const spirv_cross::Compiler* Shader::GetSpirvCompiler() const {
	return m_spirvCompiler.get();
}

Shader::Type Shader::GetType() const {
	return m_type;
	
}

VkShaderStageFlags Shader::GetVkType() const {
	switch (m_type)
	{
	case Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	default: 
		throw std::runtime_error("[Shader] Forgot to implement other shader type");
	}
}

VkShaderModule Shader::GetVkShaderModule() const {
	return m_shaderModule;
}


std::vector<uint32_t> Shader::ReadSpirvData(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error("[Shader] Could not load shader: " + path);
	}

	const size_t fileSize = file.tellg();
	if (fileSize % sizeof(uint32_t) != 0) {
		throw std::runtime_error("[Shader] Spirv shader file " + path + " is not 4 byte aligned");
	}

	std::vector<uint32_t> buffer(fileSize / 4);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));

	file.close();
	return buffer;
}

