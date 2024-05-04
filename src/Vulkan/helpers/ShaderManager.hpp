#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <volk.h>

#include "Device.hpp"

class Device;

class ShaderManager {

public:
	explicit ShaderManager(const Device* device);

	void DestroyAllShaders();

	VkShaderModule LoadShader(const std::string& path);

private:

	const Device* m_device;
	std::unordered_map<std::string, VkShaderModule> shaderMap_{};

	static std::vector<char> ReadFile(const std::string& path);
};
