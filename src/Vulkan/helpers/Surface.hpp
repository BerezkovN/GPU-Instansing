#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

class Context;

class Surface {

public:
    Surface(const Context* context, GLFWwindow* window);
    void Destroy();

    VkSurfaceKHR GetVkSurface() const;

private:
    const Context* m_context;
    VkSurfaceKHR m_surface;
};


