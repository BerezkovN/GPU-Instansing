#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

class App;

class Surface {

public:
    Surface(const App* app, GLFWwindow* window);
    void Destroy();

    VkSurfaceKHR GetVkSurface() const;

private:
    const App* m_app;
    VkSurfaceKHR m_surface;
};


