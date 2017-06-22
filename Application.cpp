#include "PrecompiledHeader.h"
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


// Callback that will be invoked on errors in validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationErrorCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData) 
{
    std::cerr << "Validation error:  " << msg << std::endl;
    return VK_FALSE;
}


// Run the application - initialize, run the main loop, cleanup at the end.
void Application::Run() {
    // initialize the application window
    InitWindow();
    // initialize the Vulkan API
	InitVulkan();
    // program's main loop
    MainLoop();
    // clean up Vulkan API and destroy the application window
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
    // set the validation debug callback
    SetupValidationErrorCallback();
    // select the graphics card to use
    SelectPhysicalDevice();
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
    // remove the validation callback
    DestroyValidationErrorCallback();
    
    // destroy the vulkan instance
	vkDestroyInstance(instance, nullptr);

	// clOSe the window
	glfwDestroyWindow(window);

	// shut down GLFW
	glfwTerminate();
}


// Create the Vulkan instance
void Application::CreateInstance() {

	// before trying to create the instance, check if all required extensions are supported
    std::vector<const char*> requiredExtensions;
    GetRequiredExtensions(requiredExtensions);
	CheckExtensionSupport(requiredExtensions);

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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

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


// Get the ulkan extensions required for the applciation to work.
void Application::GetRequiredExtensions(std::vector<const char*> &requiredExtensions) {
    requiredExtensions.clear();

    // get the info on vulkan extension glfw needs to interface with the window system
    unsigned int glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int extensionIndex = 0; extensionIndex < glfwExtensionCount; ++extensionIndex) {
        requiredExtensions.push_back(glfwExtensions[extensionIndex]);
    }

    if (enableValidationLayers) {
        requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
}


// Check if all required extensions are supported
void Application::CheckExtensionSupport(const std::vector<const char*> &requiredExtensions) {
	// get the number of supported extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// prepare a vector to hold the extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	// get the extension details
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// go through all required extensions
	for (const char *extensionName: requiredExtensions) {
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

// Set up the validation error callback
void Application::SetupValidationErrorCallback() {
    // if validation layers are not enable, don't try to set up the callback
    if (!enableValidationLayers) {
        return;
    }
    // prepare the struct to create the callback
    VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
    // set the type of the struct
    callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    // enable the callback for errors and warnings
    callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    // set the function pointer
    callbackInfo.pfnCallback = ValidationErrorCallback;

    // the function that creates the actual callback has to be obtained through vkGetInstanceProcAddr
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    // create the callback, and throw an exception if creation fails
    if (vkCreateDebugReportCallbackEXT == nullptr || vkCreateDebugReportCallbackEXT(instance, &callbackInfo, nullptr, &validationCallback) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up the validation layer debug callback");
    }
}


// Destroy the validation callbacks (on application end)
void Application::DestroyValidationErrorCallback() {
    // if validation layers are not enable, don't try to set up the callback
    if (!enableValidationLayers) {
        return;
    }
    // get the pointer to the destroy function
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    // if failed getting the function, throw an exception
    if (vkDestroyDebugReportCallbackEXT == nullptr) {
        throw std::runtime_error("Failed to destroy the validation callback");
    }
    vkDestroyDebugReportCallbackEXT(instance, validationCallback, nullptr);
}


// Select the physical device (graphics card) to render on
void Application::SelectPhysicalDevice() {
    // enumerate the available physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // if there are no physical devices, we can't render, so throw
    if (deviceCount == 0) {
        throw std::runtime_error("No available physical devices");
    }

    // get info for all physical devices
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // find the first physical device that fits the needs
    for (const VkPhysicalDevice &device : physicalDevices) {
        if (IsDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    // if no suitable physical device was found, throw
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable physical device found");
    }
}


// Does the device support all required features?
bool Application::IsDeviceSuitable(const VkPhysicalDevice &device) const {
    // get the data for properties of this device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // get the data about supported features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // NOTE: This is only an example of device property and feature selection, the real implementation would be more elaborate
    // and would probably select the best device available
    // the application requires a discrete GPU and geometry shader support
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader) {
        return true;
    }
    return false;
}
