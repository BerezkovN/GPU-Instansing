cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project (GfxGpuInstancing LANGUAGES CXX)

file(GLOB_RECURSE GPU_INSTANCING_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.*)
add_executable (GfxGpuInstancing WIN32 ${GPU_INSTANCING_SOURCES})

target_include_directories(GfxGpuInstancing PUBLIC
	${GLM_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
	${SLANG_INCLUDE_DIRS}
)

target_link_libraries(GfxGpuInstancing PUBLIC
	${GLM_LIBRARIES}
	${GLFW_LIBRARIES}
	${IMGUI_LIBRARIES}
	${SLANG_LIBRARIES}
)

set_target_properties(GfxGpuInstancing PROPERTIES UNITY_BUILD TRUE)
set_target_properties(GfxGpuInstancing PROPERTIES UNITY_BUILD_MODE BATCH)
set_target_properties(GfxGpuInstancing PROPERTIES UNITY_BUILD_BATCH_SIZE 4)

target_compile_definitions(GfxGpuInstancing PRIVATE IDE_ASSET_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/assets")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GPU_INSTANCING_SOURCES})