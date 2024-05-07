#pragma once

#include <iostream>
#include <exception>
#include <string>
#include <vector>
#include <chrono>

// Using volk instead of vulkan/vulkan.h
#include <volk.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <imgui.h>

#include <spdlog/spdlog.h>