#include "Surface.hpp"

#include "../pch.hpp"

#include "Context.hpp"

Surface::Surface(const Context* context, GLFWwindow* window) {

    m_context = context;

    m_surface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(m_context->GetVkInstance(), window, nullptr, &m_surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Surface] Could not create Vulkan surface: " + std::to_string(result));
    }
}

void Surface::Destroy() {
    vkDestroySurfaceKHR(m_context->GetVkInstance(), m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
}

VkSurfaceKHR Surface::GetVkSurface() const {
    return m_surface;
}
