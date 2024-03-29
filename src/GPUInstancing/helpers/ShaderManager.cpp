#include "ShaderManager.hpp"

#include <fstream>
#include <filesystem>
#include <ranges>

ShaderManager::ShaderManager(const VkDevice vkLogicalDevice) : vkLogicalDevice_(vkLogicalDevice) {}

void ShaderManager::DestroyAllShaders() {

	for (const auto& shaderModule : shaderMap_ | std::views::values) {
		vkDestroyShaderModule(vkLogicalDevice_, shaderModule, nullptr);
	}

	shaderMap_.clear();
}

VkShaderModule ShaderManager::LoadShader(const std::string& path) {

	std::vector<char> sprvBinary = ShaderManager::ReadFile(path);

	const VkShaderModuleCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sprvBinary.size(),
		.pCode = reinterpret_cast<uint32_t*>(sprvBinary.data())
	};

	VkShaderModule shaderModule;
	const VkResult result = vkCreateShaderModule(vkLogicalDevice_, &createInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[ShaderManager] Could not create shader module: " + std::to_string(result));
	}

	shaderMap_[path] = shaderModule;

	return shaderModule;
}

std::vector<char> ShaderManager::ReadFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		std::cout << std::filesystem::current_path() << '\n';
		throw std::runtime_error("[ShaderManager] Could not load shader: " + path);
	}

	const size_t fileSize = file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

	file.close();
	return buffer;
}
