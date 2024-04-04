
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

set(VOLK_DIR "${THIRD_PARTY_DIR}/volk")
set(GLM_DIR "${THIRD_PARTY_DIR}/glm")
set(GLFW_DIR "${THIRD_PARTY_DIR}/glfw")
set(IMGUI_DIR "${THIRD_PARTY_DIR}/imgui")

# Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)

# Volk
if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
endif()
add_subdirectory(${VOLK_DIR})
set(VULKAN_INCLUDE_DIRS Vulkan::Headers)
set(VULKAN_LIBRARIES volk)

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
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}")
set(IMGUI_LIBRARIES imgui)

# GLM
add_subdirectory(${GLM_DIR})
set(GLM_INCLUDE_DIRS ${GLM_DIR})
set(GLM_LIBRARIES glm::glm)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR})
set(GLFW_INCLUDE_DIRS ${GLFW_DIR})
set(GLFW_LIBRARIES glfw)