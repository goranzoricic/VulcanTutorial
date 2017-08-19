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
    // create a window with the required dimensions
    CreateWindow(dimWidth, dimHeight);
    // create the vulkan instance
    CreateInstance();
    // set the validation debug callback
    SetupValidationErrorCallback();
    // create the window surface
    CreateSurface();
    // select the graphics card to use
    SelectPhysicalDevice();
    // create the logical device
    CreateLogicalDevice();
    // create the swap chain
    CreateSwapChain();
    // create image views
    CreateImageViews();
	// create the render pass
	CreateRenderPass();
	// create the graphics pipeline
	CreateGraphicsPipeline();

    return true;
}


// Destroy the API. Returns true if successfull.
bool GfxAPIVulkan::Destroy() {
	// destroy the pipeline layout
	vkDestroyPipelineLayout(vkdevLogicalDevice, vkgpipePipelineLayout, nullptr);
	// destroy the render pass
	vkDestroyRenderPass(vkdevLogicalDevice, vkpassRenderPass, nullptr);
	// destroy the image views
    DestroyImageViews();
    // destroy the swap chain
    vkDestroySwapchainKHR(vkdevLogicalDevice, swcSwapChain, nullptr);
    // destroy the logical devics
    vkDestroyDevice(vkdevLogicalDevice, nullptr);
    // remove the validation callback
    DestroyValidationErrorCallback();
    // destroy the window surface
    vkDestroySurfaceKHR(vkiInstance, sfcSurface, nullptr);
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
}


// Create the Vulkan instance
void GfxAPIVulkan::CreateInstance() {

    // before trying to create the instance, check if all required extensions are supported
    std::vector<const char*> astrRequiredExtensions;
    GetRequiredInstanceExtensions(astrRequiredExtensions);
    CheckInstanceExtensionSupport(astrRequiredExtensions);

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
    VkInstanceCreateInfo ciInstance = {};
    // type of the create info struct
    ciInstance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // pointer to the appInfo
    ciInstance.pApplicationInfo = &appInfo;
    // set the exteosion info
    ciInstance.enabledExtensionCount = static_cast<uint32_t>(astrRequiredExtensions.size());
    ciInstance.ppEnabledExtensionNames = astrRequiredExtensions.data();

    // if validation layers are enabled
    if (Options::Get().ShouldUseValidationLayers()) {
        // set the number and list of names of layers to enable
        ciInstance.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        ciInstance.ppEnabledLayerNames = validationLayers.data();
        // else, no layers enabled
    }
    else {
        ciInstance.enabledLayerCount = 0;
    }

    // create the vulkan instance
    VkResult result = vkCreateInstance(&ciInstance, nullptr, &vkiInstance);

    // if the instance wasn't created successfully, throw
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a Vulkan instance");
    }
}


// Get the Vulkan instance extensions required for the applciation to work.
void GfxAPIVulkan::GetRequiredInstanceExtensions(std::vector<const char*> &astrRequiredExtensions) const {
    astrRequiredExtensions.clear();
    
    // get the info on vulkan extension glfw needs to interface with the window system
    unsigned int glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int extensionIndex = 0; extensionIndex < glfwExtensionCount; ++extensionIndex) {
        astrRequiredExtensions.push_back(glfwExtensions[extensionIndex]);
    }

    if (Options::Get().ShouldUseValidationLayers()) {
        astrRequiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
}


// Check if all required instance extensions are supported
void GfxAPIVulkan::CheckInstanceExtensionSupport(const std::vector<const char*> &astrRequiredExtensions) const {
    // get the number of supported extensions
    uint32_t ctExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ctExtensions, nullptr);

    // prepare a vector to hold the extensions
    std::vector<VkExtensionProperties> aAvailableExtensions(ctExtensions);
    // get the extension details
    vkEnumerateInstanceExtensionProperties(nullptr, &ctExtensions, aAvailableExtensions.data());

    // go through all required extensions
    for (const char *strExtension : astrRequiredExtensions) {
        bool bFound = false;
        // search for the extension in the list of supported extensions
        for (const auto &propsExtension : aAvailableExtensions) {
            // if found, mark it and exit
            if (strcmp(strExtension, propsExtension.extensionName) == 0) {
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


// Get the Vulkan device extensions required for the applciation to work.
void GfxAPIVulkan::GetRequiredDeviceExtensions(std::vector<const char*> &astrRequiredExtensions) const {
    astrRequiredExtensions.clear();

    // swap chain extension is needed to be able to present images
    astrRequiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}


// Check if all required device extensions are supported
void GfxAPIVulkan::CheckDeviceExtensionSupport(const VkPhysicalDevice &device, const std::vector<const char*> &astrRequiredExtensions) const {
    // get the number of supported extensions
    uint32_t ctExtensions = 0;
    vkEnumerateDeviceExtensionProperties(device,nullptr, &ctExtensions, nullptr);

    // prepare a vector to hold the extensions
    std::vector<VkExtensionProperties> aAvailableExtensions(ctExtensions);
    // get the extension details
    vkEnumerateDeviceExtensionProperties(device, nullptr, &ctExtensions, aAvailableExtensions.data());

    // go through all required extensions
    for (const char *strExtension : astrRequiredExtensions) {
        bool bFound = false;
        // search for the extension in the list of supported extensions
        for (const auto &propsExtension : aAvailableExtensions) {
            // if found, mark it and exit
            if (strcmp(strExtension, propsExtension.extensionName) == 0) {
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
    VkDebugReportCallbackCreateInfoEXT ciCallback = {};
    // set the type of the struct
    ciCallback.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    // enable the callback for errors and warnings
    ciCallback.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    // set the function pointer
    ciCallback.pfnCallback = ValidationErrorCallback;

    // the function that creates the actual callback has to be obtained through vkGetInstanceProcAddr
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkiInstance, "vkCreateDebugReportCallbackEXT");
    // create the callback, and throw an exception if creation fails
    if (vkCreateDebugReportCallbackEXT == nullptr || vkCreateDebugReportCallbackEXT(vkiInstance, &ciCallback, nullptr, &clbkValidation) != VK_SUCCESS) {
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


// Create the surface to present render buffers to.
void GfxAPIVulkan::CreateSurface() {
    if (glfwCreateWindowSurface(vkiInstance, _wndWindow->GetHandle(), nullptr, &sfcSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the window surface");
    }
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
bool GfxAPIVulkan::IsDeviceSuitable(const VkPhysicalDevice &device) {
    // get the data for properties of this device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // get the data about supported features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // NOTE: This is only an example of device property and feature selection, the real implementation would be more elaborate
    // and would probably select the best device available
    // the GfxAPIVulkan requires a discrete GPU and geometry shader support
    if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || !deviceFeatures.geometryShader) {
        return false;
    }

    // find indices of queue families needed to support all application's features.
    FindQueueFamilies(device);
    // if the queue families don't support all reqired features, the app can't work
    if (!IsQueueFamiliesSuitable()) {
        return false;
    }

    // before trying to create the instance, check if all required extensions are supported
    std::vector<const char*> astrRequiredExtensions;
    GetRequiredDeviceExtensions(astrRequiredExtensions);
    CheckDeviceExtensionSupport(device, astrRequiredExtensions);

    // get swap chain feature information
    QuerySwapChainSupport(device);
    // if the surface doesn't support any formats or present modes, the device isn't suitable
    if (aFormats.empty() || aPresentModes.empty()) {
        return false;
    }

    return true;
}


// Find indices of queue families needed to support all application's features.
void GfxAPIVulkan::FindQueueFamilies(const VkPhysicalDevice &device) {
    // enumerate the available queue families
    uint32_t ctQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &ctQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> aQueueFamilies(ctQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &ctQueueFamilies, aQueueFamilies.data());

    // find the queue families that support required features
    for (uint32_t iQueueFamily = 0; iQueueFamily < ctQueueFamilies; iQueueFamily++) {
        const auto &qfQueueFamily = aQueueFamilies[iQueueFamily];
        // if this is the first queue family that supports graphics commands, store its index
        if (iGraphicsQueueFamily < 0 && qfQueueFamily.queueCount > 0 && (qfQueueFamily.queueFlags | VK_QUEUE_GRAPHICS_BIT)) {
            iGraphicsQueueFamily = iQueueFamily;
        }

        // if this is the first queue family that supports presentation, store its index
        if (iPresentationQueueFamily < 0 && qfQueueFamily.queueCount > 0) {
            VkBool32 bPresentationSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, iQueueFamily, sfcSurface, &bPresentationSupport);
            if (bPresentationSupport) {
                iPresentationQueueFamily = iQueueFamily;
            }
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


// Collect information about swap chain feature support.
void GfxAPIVulkan::QuerySwapChainSupport(const VkPhysicalDevice &device) {
    // get the capabilieies of the surface
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, sfcSurface, &capsSurface);

    // get the supported formats
    uint32_t ctFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, sfcSurface, &ctFormats, nullptr);
    aFormats.resize(ctFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, sfcSurface, &ctFormats, aFormats.data());

    // get the supproted present modes
    uint32_t ctPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, sfcSurface, &ctPresentModes, nullptr);
    aPresentModes.resize(ctPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, sfcSurface, &ctPresentModes, aPresentModes.data());
}


// Create the swap chain to use for presenting images.
void GfxAPIVulkan::CreateSwapChain() {
    // select swap chain format, present mode and extent to use
    SelectSwapChainFormat();
    SelectSwapChainPresentMode();
    SelectSwapChainExtent();

    // select the number of images in the swap chain queue - one more than minimum, for tripple buffering
    uint32_t ctImages = capsSurface.minImageCount + 1;
    // maxImageCount of 0 indicates unlimited max images (limited by available memory)
    // if the number of images is limited to below the desired number, clamp to maximum
    if (capsSurface.maxImageCount > 0 && ctImages > capsSurface.maxImageCount) {
        ctImages = capsSurface.maxImageCount;
    }

    // prepare the description of the swap chain to be created
    VkSwapchainCreateInfoKHR ciSwapChain = {};
    ciSwapChain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ciSwapChain.surface = sfcSurface;

    // fill in the info collected earlier
    ciSwapChain.minImageCount = ctImages;
    ciSwapChain.imageFormat = sfmtFormat.format;
    ciSwapChain.imageColorSpace = sfmtFormat.colorSpace;
    ciSwapChain.imageExtent = sexExtent;

    // specify the present mode and mark that clipped pixels (e.g. behind another window) are not important
    ciSwapChain.presentMode = spmPresentMode;
    ciSwapChain.clipped = VK_TRUE;

    // image has only one layer (more is used for stereoscopic 3D)
    ciSwapChain.imageArrayLayers = 1;
    // this specifies that this image will be rendered to directly
    ciSwapChain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // prepare queue familiy indices to be given to Vulkan
    uint32_t aQueueFamilyIndices[] = { (uint32_t)iGraphicsQueueFamily, (uint32_t)iPresentationQueueFamily };

    // if the same queue family is used for graphics commands and presentation
    if (iGraphicsQueueFamily == iPresentationQueueFamily) {
        // flag that the image can be owned exclusively by one queue family
        // this means that ownership must be transfered explicitly to another queue family if it becomes neccessary
        // this mode gives best performance
        ciSwapChain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ciSwapChain.queueFamilyIndexCount = 0;
        ciSwapChain.pQueueFamilyIndices = nullptr;
    // else, if graphic commands and presentation will be handled by different queue families
    } else {
        // mark that multiple queue families will need concurrent access to the swap chain images
        ciSwapChain.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        // send the queue family info to the API
        ciSwapChain.queueFamilyIndexCount = 2;
        ciSwapChain.pQueueFamilyIndices = aQueueFamilyIndices;
    }

    // we can request that a transform is applied to the image before presentation
    // specifying that the current transform should be used means that no transform will be applied
    ciSwapChain.preTransform = capsSurface.currentTransform;

    // the image should be presented as opaque, no alpha blending
    ciSwapChain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // no old swapchain
    // in some cases (e.g. window is resized) the swap chain must be recreated. Then, the handle to the old swap chain
    // must be set. 
    ciSwapChain.oldSwapchain = VK_NULL_HANDLE;

    // create the swap chain
    if (vkCreateSwapchainKHR(vkdevLogicalDevice, &ciSwapChain, nullptr, &swcSwapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the swap chain");
    }

    // get the handles to swap chain images
    vkGetSwapchainImagesKHR(vkdevLogicalDevice, swcSwapChain, &ctImages, nullptr);
    aimgImages.resize(ctImages);
    vkGetSwapchainImagesKHR(vkdevLogicalDevice, swcSwapChain, &ctImages, aimgImages.data());
}


// Select the swap chain format to use.
void GfxAPIVulkan::SelectSwapChainFormat() {
    // if the API returmed VK_FORMAT_UNDEFINED as the only supported format, that means that the surface 
    // doesn't care which format is use, so we pick the one that suits us best
    // NOTE: look into using VK_COLOR_SPACE_SCRGB_LINEAR_EXT instead
    if (aFormats.size() == 1 && aFormats[0].format == VK_FORMAT_UNDEFINED) {
        sfmtFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SCRGB_NONLINEAR_EXT };
        return;
    }

    // otherwise, try to find the desired format among the returned formats
    for (const auto &format : aFormats) {
        if (format.colorSpace == VK_COLOR_SPACE_SCRGB_NONLINEAR_EXT && format.format == VK_FORMAT_B8G8R8A8_UNORM) {
            sfmtFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SCRGB_NONLINEAR_EXT };
            return;
        }
    }

    // if that also failed, just use the first format available and hope for the best
    // NOTE: look into how to improve this
    sfmtFormat = aFormats[0];
}


// Select the presentation mode to use.
void GfxAPIVulkan::SelectSwapChainPresentMode() {
    // default to immediate presentation mode
    spmPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // go through all supported present modes
    for (const auto &pmPresentMode : aPresentModes) {
        // if the mailbox mode is available, use it (for tripple buffering)
        if (pmPresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            spmPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            return;
        // if the immediate mode is supported, use that (because some drivers don't support VK_PRESENT_MODE_FIFO_KHR correctly)
        // NOTE: look into this
        } else if (pmPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            spmPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            // do not return, in case the mailbox mode is listed after this one in the array
        }
    }
}


// Select the swap chain extent to use.
void GfxAPIVulkan::SelectSwapChainExtent() {
    // uint32_max is set to width and height to signal matching to window dimensions
    if (capsSurface.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        sexExtent = capsSurface.currentExtent;
        return;
    }

    // otherwise, try to fit the extent to the window size as much as possible
    sexExtent.width = std::max(capsSurface.minImageExtent.width, std::min(capsSurface.maxImageExtent.width, _wndWindow->GetWidth()));
    sexExtent.height = std::max(capsSurface.minImageExtent.height, std::min(capsSurface.maxImageExtent.height, _wndWindow->GetHeight()));
}


// Create the image views needed to acces swap chain images.
void GfxAPIVulkan::CreateImageViews() {
    // resize the array to the correct number of views
    aimgvImageViews.resize(aimgImages.size());

    // prepare the create info
    VkImageViewCreateInfo ciImageView = {};
    ciImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

    // use it as a 2D texture (could be 1D, 2D, 3D, cube map)
    ciImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // set the format
    ciImageView.format = sfmtFormat.format;

    // set the channel mappings, use defaults
    // this allowes to bind various things to channels - swap channels around, 0, 1...
    ciImageView.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ciImageView.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ciImageView.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ciImageView.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // specify that thi should be a color target, without any layers or mip maps
    ciImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ciImageView.subresourceRange.baseMipLevel = 0;
    ciImageView.subresourceRange.levelCount = 1;
    ciImageView.subresourceRange.baseArrayLayer = 0;
    ciImageView.subresourceRange.layerCount = 1;

    // for each swap chain image, create the view
    for (size_t iImage = 0; iImage < aimgImages.size(); ++iImage) {
        // set the image handle
        ciImageView.image = aimgImages[iImage];
        // create the image view
        if (vkCreateImageView(vkdevLogicalDevice, &ciImageView, nullptr, &aimgvImageViews[iImage]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create an image view");
        }
    }
}


// Destroy the image views.
void GfxAPIVulkan::DestroyImageViews() {
    // for each swap chain image, create the view
    for (VkImageView &imgvView : aimgvImageViews) {
        vkDestroyImageView(vkdevLogicalDevice, imgvView, nullptr);
    }
}


// Create the logical device the application will use.
void GfxAPIVulkan::CreateLogicalDevice() {

    // description of queues that should be created
    std::vector<VkDeviceQueueCreateInfo> aciQueueCreateInfos;
    std::set<int> setQueueFamilies = { iGraphicsQueueFamily, iPresentationQueueFamily };

    float queuePriority = 1.0f;
    for (int iQueueFamily : setQueueFamilies) {
        VkDeviceQueueCreateInfo ciQueueCreateInfo = {};
        ciQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // create just the graphics command queue
        ciQueueCreateInfo.queueCount = 1;
        ciQueueCreateInfo.queueFamilyIndex = iQueueFamily;
        // set the queue priority
        ciQueueCreateInfo.pQueuePriorities = &queuePriority;
        // store the queue info into the array
        aciQueueCreateInfos.push_back(ciQueueCreateInfo);
    }

    // descroption of the logical device to create
    VkDeviceCreateInfo ciLogicalDeviceCreateInfo = {};
    ciLogicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // set the queue create info
    ciLogicalDeviceCreateInfo.pQueueCreateInfos = aciQueueCreateInfos.data();
    ciLogicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(setQueueFamilies.size());

    // list the needed device features
    // NOTE: not specifying any for now, will revisit later
    VkPhysicalDeviceFeatures deviceFeatures = {};
    ciLogicalDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // enable the required extensions
    std::vector<const char*> astrRequiredExtensions;
    GetRequiredDeviceExtensions(astrRequiredExtensions);
    ciLogicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(astrRequiredExtensions.size());
    ciLogicalDeviceCreateInfo.ppEnabledExtensionNames = astrRequiredExtensions.data();

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
    if (vkCreateDevice(vkdevPhysicalDevice, &ciLogicalDeviceCreateInfo, nullptr, &vkdevLogicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the logical device");
    }

    // retreive the handle to the graphics queue
    vkGetDeviceQueue(vkdevLogicalDevice, iGraphicsQueueFamily, 0, &qGraphicsQueue);
}


// Create the render pass.
void GfxAPIVulkan::CreateRenderPass() {
	// describe the attachment used for the render pass
	VkAttachmentDescription descColorAttachment = {};
	// color format is the same as the one in the swap chain
	descColorAttachment.format = sfmtFormat.format;
	// no multisampling, use one sample
	descColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// the buffer should be cleared to a constant at the start
	descColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// rendered contents need to be stored so that thay can be used afterwards
	descColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// the initial layout of the image is not important
	descColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// final layout needs to be presented in the swap chain
	descColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// describe the attachment reference
	VkAttachmentReference refAttachment = {};
	// only one attachment, bind to input 0
	refAttachment.attachment = 0;
	// the attachment will function as a color buffer
	refAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// describe the subpass needed
	VkSubpassDescription descSubPass = {};
	// this is a graphics subpass, not a compute one
	descSubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	// bind the attachment to this render pass
	descSubPass.colorAttachmentCount = 1;
	descSubPass.pColorAttachments = &refAttachment;

	// description of the render pass to create
	VkRenderPassCreateInfo ciRenderPass = {};
	ciRenderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	// bind the color attachment
	ciRenderPass.attachmentCount = 1;
	ciRenderPass.pAttachments = &descColorAttachment;
	// bind the subpass
	ciRenderPass.subpassCount = 1;
	ciRenderPass.pSubpasses = &descSubPass;

	// finally, create the render pass
	if (vkCreateRenderPass(vkdevLogicalDevice, &ciRenderPass, nullptr, &vkpassRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the render pass");
	}

}


// Create the graphics pipeline.
void GfxAPIVulkan::CreateGraphicsPipeline() {
	// describe the vertex program inputs
	VkPipelineVertexInputStateCreateInfo ciVertexInput = {};
	ciVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// since all vertexes info is hardcode in the shader, there are no bindings
	ciVertexInput.vertexBindingDescriptionCount = 0;
	ciVertexInput.pVertexBindingDescriptions = nullptr;
	// also, there are no attributes
	ciVertexInput.vertexAttributeDescriptionCount = 0;
	ciVertexInput.pVertexAttributeDescriptions = nullptr;

	// describe the topology and if primitive restart will be used
	VkPipelineInputAssemblyStateCreateInfo ciInputAssembly;
	// triangle list will be used
	ciInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// no primitive restart (if this is set to TRUE, index of 0xFFFF/0xFFFFFFFF means that the next index starts a new primitive)
	ciInputAssembly.primitiveRestartEnable = VK_FALSE;

	// describe the rendering viewport
	VkViewport vpViewport = {};
	// viweport coves the full screen
	vpViewport.x = 0.0f;
	vpViewport.y = 0.0f;
	vpViewport.width = (float) sexExtent.width;
	vpViewport.height = (float) sexExtent.height;
	// full range of depths
	vpViewport.minDepth = 0.0f;
	vpViewport.maxDepth = 1.0f;

	// set up the scissor to also cover the full screen
	VkRect2D rectScissor = {};
	rectScissor.offset = { 0, 0 };
	rectScissor.extent = sexExtent;

	// describe the viewport state for the pipeline
	VkPipelineViewportStateCreateInfo ciViewportState = {};
	ciViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	// bind the viewport description (can be multiple in some cases)
	ciViewportState.viewportCount = 1;
	ciViewportState.pViewports = &vpViewport;
	// bind the scissor (also, can be multiple)
	ciViewportState.scissorCount = 1;
	ciViewportState.pScissors = &rectScissor;


	// describe the rasterizer - how the vertex info is converted into fragments that will be passed to fragment programs
	VkPipelineRasterizationStateCreateInfo ciRasterizationState = {};
	// fragments should be discarded if they are not between near and far planes
	ciRasterizationState.depthClampEnable = VK_FALSE;
	// geometry should be rasterized (FALSE means no fragments will be produced)
	ciRasterizationState.rasterizerDiscardEnable = VK_FALSE;
	// we want polygons to be filled with fragments (as opposed to just points or lines)
	ciRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	// thickness of lines, in number of fragments
	ciRasterizationState.lineWidth = 1.0f;
	// enable back face culling
	ciRasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	// forward facing faces use clockwise vetex winding
	ciRasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// no depth bias
	ciRasterizationState.depthBiasEnable = VK_FALSE;
	ciRasterizationState.depthBiasConstantFactor = 0.0f;
	ciRasterizationState.depthBiasClamp = 0.0f;
	ciRasterizationState.depthBiasSlopeFactor = 0.0f;


	// describe the multisampling configuration
	VkPipelineMultisampleStateCreateInfo ciMultisampling = {};
	ciMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// multisampling is disabled
	ciMultisampling.sampleShadingEnable = VK_FALSE;
	// set the rest of multisampling values to the simplest
	// NOTE: they are not described in the tutorial, so no comments for them at this point
	ciMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ciMultisampling.minSampleShading = 1.0f;
	ciMultisampling.pSampleMask = nullptr;
	ciMultisampling.alphaToCoverageEnable = VK_FALSE;
	ciMultisampling.alphaToOneEnable = VK_FALSE;


	// describe how the color output of a fragment program is blended with the frame buffer
	VkPipelineColorBlendAttachmentState descColorBlendState = {};
	// fragments wi write RGBA channels
	descColorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// blending is disabled, fragment color will overwrite the framebuffer value
	descColorBlendState.blendEnable = VK_FALSE;
	// setting the default color blend params
	descColorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	descColorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	descColorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	descColorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	descColorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	descColorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;

	// describe the color blending state of the pipeline (will include the reference to the blend state attachment)
	VkPipelineColorBlendStateCreateInfo ciColorBlendState = {};
	ciColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// disable color blending
	ciColorBlendState.logicOpEnable = VK_FALSE;
	// set 'copy' as the bitwase operation
	ciColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	// bind the color blend attachment
	ciColorBlendState.attachmentCount = 1;
	ciColorBlendState.pAttachments = &descColorBlendState;
	// set blending constants
	ciColorBlendState.blendConstants[0] = 0.0f;
	ciColorBlendState.blendConstants[1] = 0.0f;
	ciColorBlendState.blendConstants[2] = 0.0f;
	ciColorBlendState.blendConstants[3] = 0.0f;


	// finally, describe the graphics pipeline
	VkPipelineLayoutCreateInfo ciPipeline = {};
	ciPipeline.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ciPipeline.setLayoutCount = 0;
	ciPipeline.pSetLayouts = nullptr;
	ciPipeline.pushConstantRangeCount = 0;
	ciPipeline.pPushConstantRanges = 0;

	// create the pipeline
	if (vkCreatePipelineLayout(vkdevLogicalDevice, &ciPipeline, nullptr, &vkgpipePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the pipeline layout!");
	}
}
