
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

set(VOLK_DIR "${THIRD_PARTY_DIR}/volk")
set(GLFW_DIR "${THIRD_PARTY_DIR}/glfw")
set(IMGUI_DIR "${THIRD_PARTY_DIR}/imgui")
set(GLM_DIR "${THIRD_PARTY_DIR}/glm")
set(SLANG_DIR "${THIRD_PARTY_DIR}/slang")
set(SPDLOG_DIR "${THIRD_PARTY_DIR}/spdlog")

# Slang and GFX
set(SLANG_ENABLE_EXAMPLES OFF)
set(SLANG_LIB_TYPE STATIC)
add_subdirectory(${SLANG_DIR})
set(SLANG_INCLUDE_DIRS ${SLANG_DIR})
set(SLANG_LIBRARIES core slang gfx gfx-util platform)

function(print_sources target_name)
    get_target_property(target_sources ${target_name} SOURCES)
    if(NOT target_sources)
        message(WARNING "Target ${target_name} has no sources.")
        return()
    endif()
    message("Sources of target ${target_name}:")
    foreach(source ${target_sources})
        message("${source}")
    endforeach()
endfunction()

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
if (NOT TARGET imgui)
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
endif()

set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}")
set(IMGUI_LIBRARIES imgui)

# GLM
add_subdirectory(${GLM_DIR})
set(GLM_INCLUDE_DIRS ${GLM_DIR})
set(GLM_LIBRARIES glm::glm)

# SPDLOG
add_subdirectory(${SPDLOG_DIR})
set(SPDLOG_INCLUDE_DIRS "${SPDLOG_DIR}/include")
set(SPDLOG_LIBRARIES spdlog::spdlog)
