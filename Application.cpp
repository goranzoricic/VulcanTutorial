#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>

// enable validation layers only in debug builds
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// dimensions of the application window
const int WIDTH = 1920;
const int HEIGHT = 1080;

// list of validation layers' names that we want to enable
const std::vector<const char*> validationLayers = {
	// this is a standard set of validation layers, not a single layer
	"VK_LAYER_LUNARG_standard_validation"
};


// Run the application - initialize, run the main loop, cleanup at the end.
void Application::Run() {
	InitWindow();
	InitVulkan();
	MainLoop();
	Cleanup();
}


// Initialize the application window.
void Application::InitWindow() {
	// init the GLFW library
	glfwInit();

	// prevent GLFW from creating an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// window is not resizable, to avoid dealing with that
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// create the window
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}


// Initialize the Vulkan API
void Application::InitVulkan() {
	// create the vulkan instance
	CreateInstance();
}


// Program's main loop
void Application::MainLoop() {
	// loop until the user closes the window
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}


// Clean up Vulkan API and destroy the application window
void Application::Cleanup() {
	// destroy the vulkan instance
	vkDestroyInstance(instance, nullptr);

	// clOSe the window
	glfwDestroyWindow(window);

	// shut down GLFW
	glfwTerminate();
}


// Create the Vulkan instance
void Application::CreateInstance() {
	// get the info on vulkan extension glfw needs to interface with the window system
	unsigned int glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// before trying to create the instance, check if all required extensions are supported
	CheckExtensionSupport(glfwExtensionCount, glfwExtensions);

	// if validation layers are enabled, set them up
	SetupValidationLayers();

	// application info contains data about the application that is passed to the graphics driver
	// to inform its behavior
	VkApplicationInfo appInfo = {};
	// type of app info 
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	// name of the application
	appInfo.pApplicationName = "Vulkan tutorial - triangle";
	// version of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	// name of the engine that created the application (UE, Unity? not sure what this is exactly)
	appInfo.pEngineName = "No Enging";
	// version of the engine
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	// version of the Vulkan API to use
	appInfo.apiVersion = VK_API_VERSION_1_0;


	// create the info about which extensions and validators we want to use
	VkInstanceCreateInfo createInfo = {};
	// type of the create info struct
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// pointer to the appInfo
	createInfo.pApplicationInfo = &appInfo;
	// set the exteosion info
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	// if validation layers are enabled
	if (enableValidationLayers) {
		// set the number and list of names of layers to enable
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		// else, no layers enabled
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	// create the vulkan instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	// if the instance wasn't created successfully, throw
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Vulkan instance");
	}
}


// Check if all required extensions are supported
void Application::CheckExtensionSupport(unsigned int glfwExtensionCount, const char **glfwExtensions) {
	// get the number of supported extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// prepare a vector to hold the extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	// get the extension details
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// go through all required extensions
	for (unsigned int extensionIndex = 0; extensionIndex < glfwExtensionCount; ++extensionIndex) {
		const char *extensionName = glfwExtensions[extensionIndex];
		bool bFound = false;
		// search for the extension in the list of supported extensions
		for (const auto &extensionProperties : extensions) {
			// if found, mark it and exit
			if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
				bFound = true;
				break;
			}
		}
		// if the extension was not found, throw an exception
		if (!bFound) {
			throw std::runtime_error("Not all required extensions are supported");
		}
	}
}


// Set up the validation layers.
void Application::SetupValidationLayers() {
	if (enableValidationLayers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("Validation layers enabled but not available!");
	}
}


// Check if all requested valdiation layers are supported.
bool Application::CheckValidationLayerSupport() {
	// get thenumber of available layers
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// get all supported layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// go through all required layers
	for (const char *layerName : validationLayers) {
		// search for this layer in the list of available layers
		bool layerFound = false;
		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		// if the layer wasn't found, not all required layers are supported
		if (!layerFound) {
			return false;
		}
	}
	return true;
}
