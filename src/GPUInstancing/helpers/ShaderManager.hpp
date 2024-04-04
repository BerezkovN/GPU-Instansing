#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <volk.h>

class ShaderManager {

public:
	explicit ShaderManager(VkDevice vkLogicalDevice);

	void DestroyAllShaders();

	VkShaderModule LoadShader(const std::string& path);

private:

	VkDevice vkLogicalDevice_;
	std::unordered_map<std::string, VkShaderModule> shaderMap_{};

	static std::vector<char> ReadFile(const std::string& path);
};