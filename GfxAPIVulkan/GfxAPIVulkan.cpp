#include "../PrecompiledHeader.h"
#include "GfxAPIVulkan.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "../Options.h"
#include "../GfxAPI/Window.h"


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


// Initialize the API. Returns true if successfull.
bool GfxAPIVulkan::Initialize(uint32_t dimWidth, uint32_t dimHeight) {
    CreateWindow(dimWidth, dimHeight);
    // create the vulkan instance
    CreateInstance();
    // set the validation debug callback
    SetupValidationErrorCallback();
    // select the graphics card to use
    SelectPhysicalDevice();

    return true;
}


// Destroy the API. Returns true if successfull.
bool GfxAPIVulkan::Destroy() {
    // destroy the logical devics
    vkDestroyDevice(vkdevLogicalDevice, nullptr);
    // remove the validation callback
    DestroyValidationErrorCallback();
    // destroy the vulkan instance
    vkDestroyInstance(vkiInstance, nullptr);
    // close the window
    _wndWindow->Close();
    // shut down GLFW
    glfwTerminate();

    return true;
}


// Initialize the GfxAPIVulkan window.
void GfxAPIVulkan::CreateWindow(uint32_t dimWidth, uint32_t dimHeight) {
    // init the GLFW library
    glfwInit();

    // prevent GLFW from creating an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // window is not resizable, to avoid dealing with that
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create the window object, GLFW representation of the system window and link the two up
    _wndWindow = std::make_shared<Window>();
    GLFWwindow *wndWindow = glfwCreateWindow(dimWidth, dimHeight, "Vulkan", nullptr, nullptr);
    _wndWindow->Initialize(dimWidth, dimHeight, wndWindow);

    // create the vulkan instance
    CreateInstance();
    // set the validation debug callback
    SetupValidationErrorCallback();
    // select the graphics card to use
    SelectPhysicalDevice();
}


// Create the Vulkan instance
void GfxAPIVulkan::CreateInstance() {

    // before trying to create the instance, check if all required extensions are supported
    std::vector<const char*> requiredExtensions;
    GetRequiredExtensions(requiredExtensions);
    CheckExtensionSupport(requiredExtensions);

    // if validation layers are enabled, set them up
    SetupValidationLayers();

    // VkApplicationInfo info contains data about the application that is passed to the graphics driver
    // to inform its behavior
    VkApplicationInfo appInfo = {};
    // type of app info 
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // name of the GfxAPIVulkan
    appInfo.pApplicationName = "Vulkan tutorial - triangle";
    // version of the GfxAPIVulkan
    appInfo.applicationVersion= VK_MAKE_VERSION(1, 0, 0);
    // name of the engine that created the GfxAPIVulkan (UE, Unity? not sure what this is exactly)
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
    if (Options::Get().ShouldUseValidationLayers()) {
        // set the number and list of names of layers to enable
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        // else, no layers enabled
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    // create the vulkan instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &vkiInstance);

    // if the instance wasn't created successfully, throw
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a Vulkan instance");
    }
}


// Get the ulkan extensions required for the applciation to work.
void GfxAPIVulkan::GetRequiredExtensions(std::vector<const char*> &requiredExtensions) {
    requiredExtensions.clear();

    // get the info on vulkan extension glfw needs to interface with the window system
    unsigned int glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int extensionIndex = 0; extensionIndex < glfwExtensionCount; ++extensionIndex) {
        requiredExtensions.push_back(glfwExtensions[extensionIndex]);
    }

    if (Options::Get().ShouldUseValidationLayers()) {
        requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
}


// Check if all required extensions are supported
void GfxAPIVulkan::CheckExtensionSupport(const std::vector<const char*> &requiredExtensions) {
    // get the number of supported extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // prepare a vector to hold the extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    // get the extension details
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    // go through all required extensions
    for (const char *extensionName : requiredExtensions) {
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
void GfxAPIVulkan::SetupValidationLayers() {
    if (Options::Get().ShouldUseValidationLayers() && !CheckValidationLayerSupport()) {
        throw std::runtime_error("Validation layers enabled but not available!");
    }
}


// Check if all requested valdiation layers are supported.
bool GfxAPIVulkan::CheckValidationLayerSupport() {
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
void GfxAPIVulkan::SetupValidationErrorCallback() {
    // if validation layers are not enable, don't try to set up the callback
    if (Options::Get().ShouldUseValidationLayers()) {
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
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkiInstance, "vkCreateDebugReportCallbackEXT");
    // create the callback, and throw an exception if creation fails
    if (vkCreateDebugReportCallbackEXT == nullptr || vkCreateDebugReportCallbackEXT(vkiInstance, &callbackInfo, nullptr, &clbkValidation) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up the validation layer debug callback");
    }
}


// Destroy the validation callbacks (on GfxAPIVulkan end)
void GfxAPIVulkan::DestroyValidationErrorCallback() {
    // if validation layers are not enable, don't try to set up the callback
    if (Options::Get().ShouldUseValidationLayers()) {
        return;
    }
    // get the pointer to the destroy function
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vkiInstance, "vkDestroyDebugReportCallbackEXT");
    // if failed getting the function, throw an exception
    if (vkDestroyDebugReportCallbackEXT == nullptr) {
        throw std::runtime_error("Failed to destroy the validation callback");
    }
    vkDestroyDebugReportCallbackEXT(vkiInstance, clbkValidation, nullptr);
}


// Select the physical device (graphics card) to render on
void GfxAPIVulkan::SelectPhysicalDevice() {
    // enumerate the available physical devices
    uint32_t ctDevices = 0;
    vkEnumeratePhysicalDevices(vkiInstance, &ctDevices, nullptr);

    // if there are no physical devices, we can't render, so throw
    if (ctDevices == 0) {
        throw std::runtime_error("No available physical devices");
    }

    // get info for all physical devices
    std::vector<VkPhysicalDevice> aPhysicalDevices(ctDevices);
    vkEnumeratePhysicalDevices(vkiInstance, &ctDevices, aPhysicalDevices.data());

    // find the first physical device that fits the needs
    for (const VkPhysicalDevice &device : aPhysicalDevices) {
        if (IsDeviceSuitable(device)) {
            vkdevPhysicalDevice = device;
            break;
        }
    }

    // if no suitable physical device was found, throw
    if (vkdevPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable physical device found");
    }
}


// Does the device support all required features?
bool GfxAPIVulkan::IsDeviceSuitable(const VkPhysicalDevice &device) const {
    // get the data for properties of this device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // get the data about supported features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // NOTE: This is only an example of device property and feature selection, the real implementation would be more elaborate
    // and would probably select the best device available
    // the GfxAPIVulkan requires a discrete GPU and geometry shader support
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader) {
        return true;
    }

    // if the queue families don't support all reqired features, the app can't work
    if (!IsQueueFamiliesSuitable()) {
        return false;
    }

    return false;
}


// Find indices of queue families needed to support all application's features.
void GfxAPIVulkan::FindQueueFamilies() {
    // enumerate the available queue families
    uint32_t ctQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkdevPhysicalDevice, &ctQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> aQueueFamilies(ctQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(vkdevPhysicalDevice, &ctQueueFamilies, aQueueFamilies.data());

    // find the queue families that support required features
    for (uint32_t iQueueFamily = 0; iQueueFamily < ctQueueFamilies; iQueueFamily++) {
        const auto &qfQueueFamily = aQueueFamilies[iQueueFamily];
        if (iGraphicsQueueFamily < 0 && qfQueueFamily.queueCount > 0 && (qfQueueFamily.queueFlags | VK_QUEUE_GRAPHICS_BIT)) {
            iGraphicsQueueFamily = iQueueFamily;
        }
    }   
}


// Do the queue families support all required features?
bool GfxAPIVulkan::IsQueueFamiliesSuitable() const {
    if (iGraphicsQueueFamily < 0) {
        return false;
    }
    return true;
}


// Create the logical device the application will use.
void GfxAPIVulkan::CreateLogicalDevice() {
    // description of queues that should be created
    VkDeviceQueueCreateInfo ciQueueCreateInfo = {};
    ciQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // create just the graphics command queue
    ciQueueCreateInfo.queueCount = 1;
    ciQueueCreateInfo.queueFamilyIndex = iGraphicsQueueFamily;
    // set the queue priority
    float queuePriority = 1.0f;
    ciQueueCreateInfo.pQueuePriorities = &queuePriority;

    // descroption of the logical device to create
    VkDeviceCreateInfo ciLogicalDeviceCreateInfo = {};
    ciLogicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // set the queue create info
    ciLogicalDeviceCreateInfo.pQueueCreateInfos = &ciQueueCreateInfo;
    ciLogicalDeviceCreateInfo.queueCreateInfoCount = 1;

    // list the needed device features
    // NOTE: not specifying any for now, will revisit later
    VkPhysicalDeviceFeatures deviceFeatures = {};
    ciLogicalDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // no enabled extensions yet
    // NOTE: these are device specific extension, will come back to them when doing swap_chain setup
    ciLogicalDeviceCreateInfo.enabledExtensionCount = 0;    

    // if validation layers are enabled
    if (Options::Get().ShouldUseValidationLayers()) {
        // set the number and list of names of layers to enable
        ciLogicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        ciLogicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    // else, no layers enabled
    } else {
        ciLogicalDeviceCreateInfo.enabledLayerCount = 0;
    }

    // create the logical device
    if (!vkCreateDevice(vkdevPhysicalDevice, &ciLogicalDeviceCreateInfo, nullptr, &vkdevLogicalDevice)) {
        throw std::runtime_error("Failed to create the logicla device");
    }

    // retreive the handle to the graphics queue
    vkGetDeviceQueue(vkdevLogicalDevice, iGraphicsQueueFamily, 0, &qGraphicsQueue);
}
