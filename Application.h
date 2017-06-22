#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Application {
public:
    Application() : physicalDevice(VK_NULL_HANDLE) {}

    // Run the application - initialize, run the main loop, cleanup at the end.
	void Run();

private:
	// Application window
	struct GLFWwindow * window;
	// Handle to the vulkan instance
	VkInstance instance;
    // Handle to the debug callback
    VkDebugReportCallbackEXT validationCallback;
    // Physical device (graphics card) used
    VkPhysicalDevice physicalDevice;

	// Initialize the application window
	void InitWindow();
	// Initialize the Vulkan API
	void InitVulkan();
	// Program's main loop
	void MainLoop();
	// Clean up Vulkan API and destroy the application window
	void Cleanup();

	// Create the Vulkan instance
	void CreateInstance();
    // Get the ulkan extensions required for the applciation to work.
    void GetRequiredExtensions(std::vector<const char*> &requiredExtensions);
    // Check if all required extensions are supported
	void CheckExtensionSupport(const std::vector<const char*> &requiredExtensions);

    // NOTE: In the Vulkan SDK, Config directory, there is a vk_layer_settings.txt file that explains how to configure the validation layers.
	// Set up the validation layers.
	void SetupValidationLayers();
	// Check if all requested valdiation layers are supported.
	bool CheckValidationLayerSupport();
    // Set up the validation error callback
    void SetupValidationErrorCallback();
    // Destroy the validation callbacks (on application end)
    void DestroyValidationErrorCallback();

    // Select the physical device (graphics card) to render on
    void SelectPhysicalDevice();
    // Does the device support all required features?
    bool IsDeviceSuitable(const VkPhysicalDevice &device) const;
};
