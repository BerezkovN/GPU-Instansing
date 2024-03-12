
project (VulkanGPUInstancing LANGUAGES CXX)

add_executable (VulkanGPUInstancing "GPUInstancing.cpp" "GPUInstancing.hpp")

target_include_directories(VulkanGPUInstancing PUBLIC
	${GLM_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIRS}
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(VulkanGPUInstancing PUBLIC
	${GLM_LIBRARIES}
	${GLFW_LIBRARIES}
	${Vulkan_LIBRARIES}
)

