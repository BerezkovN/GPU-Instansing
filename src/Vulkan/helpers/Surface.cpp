#include "Surface.hpp"

#include "../App.hpp"

Surface::Surface(const App* app, GLFWwindow* window) {

    m_app = app;

    m_surface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(m_app->GetVkInstance(), window, nullptr, &m_surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Surface] Could not create Vulkan surface: " + std::to_string(result));
    }
}

void Surface::Destroy() {
    vkDestroySurfaceKHR(m_app->GetVkInstance(), m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
}

VkSurfaceKHR Surface::GetVkSurface() const {
    return m_surface;
}
