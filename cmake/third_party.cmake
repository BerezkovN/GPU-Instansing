
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

set(VOLK_DIR "${THIRD_PARTY_DIR}/volk")
set(GLFW_DIR "${THIRD_PARTY_DIR}/glfw")
set(IMGUI_DIR "${THIRD_PARTY_DIR}/imgui")
set(GLM_DIR "${THIRD_PARTY_DIR}/glm")
set(SLANG_DIR "${THIRD_PARTY_DIR}/slang")
set(SPDLOG_DIR "${THIRD_PARTY_DIR}/spdlog")
set(STB_IMAGE_DIR "${THIRD_PARTY_DIR}/stb_image")
set(SPIRV_CROSS_DIR "${THIRD_PARTY_DIR}/SPIRV-Cross")

# Vulkan
if (NOT TARGET Vulkan::Headers)
    find_package(Vulkan REQUIRED)
    set(VULKAN_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
endif()

# Volk
if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
endif()
add_subdirectory(${VOLK_DIR})
set(VULKAN_LIBRARIES volk)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR})
set(GLFW_INCLUDE_DIRS "${GLFW_DIR}/include")
set(GLFW_LIBRARIES glfw)

# ImGUI
add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${GLFW_INCLUDE_DIRS} ${VULKAN_INCLUDE_DIRS})
target_link_libraries(imgui PRIVATE)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES)


set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}")
set(IMGUI_LIBRARIES imgui)

print_sources(imgui)

# GLM
add_subdirectory(${GLM_DIR})
set(GLM_INCLUDE_DIRS ${GLM_DIR})
set(GLM_LIBRARIES glm::glm)

# SPDLOG
add_subdirectory(${SPDLOG_DIR})
set(SPDLOG_INCLUDE_DIRS "${SPDLOG_DIR}/include")
set(SPDLOG_LIBRARIES spdlog::spdlog)

# SBI_IMAGE
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE "${STB_IMAGE_DIR}/stb_image.h")
set(STB_IMAGE_INCLUDE_DIRS "${STB_IMAGE_DIR}")
set(STB_IMAGE_LIBRARIES stb_image)

# SPIRV-Cross
set(SPIRV_CROSS_ENABLE_GLSL     OFF CACHE BOOL "Enable GLSL support.")
set(SPIRV_CROSS_ENABLE_REFLECT  OFF CACHE BOOL "Disable JSON target support.")
set(SPIRV_CROSS_ENABLE_HLSL     OFF CACHE BOOL "Disable HLSL target support.")
set(SPIRV_CROSS_ENABLE_MSL      OFF CACHE BOOL "Disable MSL target support.")
set(SPIRV_CROSS_ENABLE_CPP      OFF CACHE BOOL "Disable C++ target support.")

set(SPIRV_CROSS_CLI             OFF CACHE BOOL "Build the CLI binary. Requires SPIRV_CROSS_STATIC.")
set(SPIRV_CROSS_ENABLE_TESTS    OFF CACHE BOOL "Enable SPIRV-Cross tests.")
set(SPIRV_CROSS_ENABLE_C_API    OFF CACHE BOOL "Enable C API wrapper support in static library.")

add_subdirectory(${SPIRV_CROSS_DIR})
set(SPIRV_CROSS_INCLUDE_DIRS "${SPRIV_CROSS_DIR}/include")
set(SPIRV_CROSS_LIBRARIES spirv-cross-core)
