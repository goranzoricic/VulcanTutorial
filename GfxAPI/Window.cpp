#include "../PrecompiledHeader.h"
#include "Window.h"
#include <GLFW/glfw3.h>


// Set the window data
void Window::Initialize(uint32_t dimWidth, uint32_t dimHeight, GLFWwindow *wndWindow) {
    assert(dimWidth > 0);
    assert(dimHeight > 0);
    assert(wndWindow != nullptr);

    _dimWidth = dimWidth;
    _dimHeight = dimHeight;
    _wndWindow = wndWindow;
}

// Should the window be closed?
bool Window::ShouldClose() {
    if (glfwWindowShouldClose(_wndWindow)) {
        return true;
    }
    return false;
}

// Process window messages.
void Window::ProcessMessages() {
    glfwPollEvents();
}


// Close the window.
void Window::Close() {
    glfwDestroyWindow(_wndWindow);
}
