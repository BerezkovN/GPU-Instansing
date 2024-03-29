cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project (VulkanGPUInstancing LANGUAGES CXX)

file(GLOB_RECURSE GPU_INSTANCING_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.*)
add_executable (VulkanGPUInstancing ${GPU_INSTANCING_SOURCES})

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

set_target_properties(VulkanGPUInstancing PROPERTIES UNITY_BUILD TRUE)
set_target_properties(VulkanGPUInstancing PROPERTIES UNITY_BUILD_MODE BATCH)
set_target_properties(VulkanGPUInstancing PROPERTIES UNITY_BUILD_BATCH_SIZE 4)

set_target_properties(VulkanGPUInstancing PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY 
	"${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GPU_INSTANCING_SOURCES})