#pragma once

#include <vulkan/vulkan.h>

class Application {
public:
	// Run the application - initialize, run the main loop, cleanup at the end.
	void Run();

private:
	// application window
	struct GLFWwindow * window;
	// handle to the vulkan instance
	VkInstance instance;

	// Initialize the application window.
	void InitWindow();
	// Initialize the Vulkan API
	void InitVulkan();
	// Program's main loop
	void MainLoop();
	// Clean up Vulkan API and destroy the application window
	void Cleanup();

	// Create the Vulkan instance
	void CreateInstance();
	// Check if all required extensions are supported
	void CheckExtensionSupport(unsigned int glfwExtensionCount, const char **glfwExtensions);
	// Set up the validation layers.
	void SetupValidationLayers();
	// Check if all requested valdiation layers are supported.
	bool CheckValidationLayerSupport();
};
