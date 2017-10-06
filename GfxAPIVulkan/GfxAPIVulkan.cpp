#include "../PrecompiledHeader.h"
#include "GfxAPIVulkan.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "../Options.h"
#include "../GfxAPI/Window.h"


// NOTE: refactor this
struct Vertex {
    glm::vec2 vecPosition;
    glm::vec3 colColor;

    // Describe to the Vulkan API how to handle Vertex data.
    static VkVertexInputBindingDescription GetBindingDescription() {
        // describe the layout of a vertex
        VkVertexInputBindingDescription descVertexInputBinding = {};
        // index of the binding in the array of bindings
        descVertexInputBinding.binding = 0;
        // number of bytes from the start of one entry to the next
        descVertexInputBinding.stride = sizeof(Vertex);
        // move to next data entry after each vertex (could be instance)
        descVertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return descVertexInputBinding;
    };

    // Describe each individual vertex attribute.
    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> adescAttributes = {};
        // set up the description of the vertex position
        // data comes from the binding 0 (set up above)
        adescAttributes[0].binding = 0;
        // data goes to the location 0 (specified in the vertex shader)
        adescAttributes[0].location = 0;
        // data is two 32bit floats (screen x, y)
        adescAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
        // offset of this attribute from the start of the data block
        adescAttributes[0].offset = offsetof(Vertex, vecPosition);

        // set up the description of the vertex color
        // data comes from the binding 0 (set up above)
        adescAttributes[1].binding = 0;
        // data goes to the location 0 (specified in the vertex shader)
        adescAttributes[1].location = 1;
        // data is three 32bit floats (red, green, blue)
        adescAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        // offset of this attribute from the start of the data block
        adescAttributes[1].offset = offsetof(Vertex, colColor);

        return adescAttributes;
    };
};

// Vertices that the drawn shape consists of.
const std::vector<Vertex> avVertices = {
    { { -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f } },
    { { 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f } },
    { { 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } },
    { { -0.5f, 0.5f },{ 1.0f, 1.0f, 1.0f } } 
};


// Indices that describe the order of vertices in shape's triangles.
const std::vector<uint16_t> aiIndices = {
    0, 1, 2,
    2, 3, 0,
};


// List of validation layers' names that we want to enable.
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

void GfxAPIVulkan::OnWindowResizedCallback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) {
        return;
    }
    dynamic_cast<GfxAPIVulkan*>(GfxAPI::Get())->OnWindowResized(window, width, width);
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
    // create descriptor set layout
    CreateDescriptorSetLayout();
    // create the graphics pipeline
    CreateGraphicsPipeline();
    // create the framebuffers
    CreateFramebuffers();
    // create the command pool
    CreateCommandPool();
    // create the vertex buffer
    CreateVertexBuffers();
    // create the index buffer
    CreateIndexBuffers();

    // allocate command buffers
    CreateCommandBuffers();

    // record the command buffers - NOTE: this is for the simple drawing from the tutorial.
    RecordCommandBuffers();

    // create the semaphores
    CreateSemaphores();

    return true;
}


// Destroy the API. Returns true if successfull.
bool GfxAPIVulkan::Destroy() {
    // wait for the logical device to finish its current batch of work
    vkDeviceWaitIdle(vkdevLogicalDevice);

    // destroy the swap chain
    DestroySwapChain();
    
    // destroy the descriptor set layout
    vkDestroyDescriptorSetLayout(vkdevLogicalDevice, vkhDescriptorSetLayout, nullptr);
    // destroy the vertex buffer
    vkDestroyBuffer(vkdevLogicalDevice, vkhVertexBuffer, nullptr);
    // release memory used by the vertex buffer
    vkFreeMemory(vkdevLogicalDevice, vkhVertexBufferMemory, nullptr);
    // destroy the vertex buffer
    vkDestroyBuffer(vkdevLogicalDevice, vkhIndexBuffer, nullptr);
    // release memory used by the vertex buffer
    vkFreeMemory(vkdevLogicalDevice, vkhIndexBufferMemory, nullptr);

    // destroy semaphores
    DestroySemaphores();
    // destoy the command pool
    vkDestroyCommandPool(vkdevLogicalDevice, vkhCommandPool, nullptr);

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

// Initialize swap chain. Called on first initialization, but also on window resize.
void GfxAPIVulkan::InitializeSwapChain() {
    // wait for the logical device to be idne
    vkDeviceWaitIdle(vkdevLogicalDevice);

    // destroy the swap chain
    DestroySwapChain();

    // create the swap chain
    CreateSwapChain();
    // create image views
    CreateImageViews();
    // create the render pass
    CreateRenderPass();
    // create descriptor set layout
    CreateDescriptorSetLayout();
    // create the graphics pipeline
    CreateGraphicsPipeline();
    // create the framebuffers
    CreateFramebuffers();
    // allocate command buffers
    CreateCommandBuffers();
    // record the command buffers - NOTE: this is for the simple drawing from the tutorial.
    RecordCommandBuffers();
}

// Destroy the swap chain.
void GfxAPIVulkan::DestroySwapChain() {
    // delete the command buffers
    if (acbufCommandBuffers.size() > 0) {
        vkFreeCommandBuffers(vkdevLogicalDevice, vkhCommandPool, (uint32_t)acbufCommandBuffers.size(), acbufCommandBuffers.data());
    }
    // destroy the framebuffers
    DestroyFramebuffers();
    // destroy the pipeline
    vkDestroyPipeline(vkdevLogicalDevice, vkgpipePipeline, nullptr);
	// destroy the pipeline layout
	vkDestroyPipelineLayout(vkdevLogicalDevice, vkplPipelineLayout, nullptr);
	// destroy the render pass
	vkDestroyRenderPass(vkdevLogicalDevice, vkpassRenderPass, nullptr);
	// destroy the image views
    DestroyImageViews();
    // destroy the swap chain
    vkDestroySwapchainKHR(vkdevLogicalDevice, swcSwapChain, nullptr);
}

// Initialize the GfxAPIVulkan window.
void GfxAPIVulkan::CreateWindow(uint32_t dimWidth, uint32_t dimHeight) {
    // init the GLFW library
    glfwInit();

    // prevent GLFW from creating an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // create the window object, GLFW representation of the system window and link the two up
    _wndWindow = std::make_shared<Window>();
    GLFWwindow *wndWindow = glfwCreateWindow(dimWidth, dimHeight, "Vulkan", nullptr, nullptr);
    _wndWindow->Initialize(dimWidth, dimHeight, wndWindow);

    // set the window resize callback
    glfwSetWindowUserPointer(wndWindow, this);
    glfwSetWindowSizeCallback(wndWindow, GfxAPIVulkan::OnWindowResizedCallback);
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

    if (Options::Get().ShouldUseValidationLayers()) {
        // the function that creates the actual callback has to be obtained through vkGetInstanceProcAddr
        auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkiInstance, "vkCreateDebugReportCallbackEXT");
        // create the callback, and throw an exception if creation fails
        if (vkCreateDebugReportCallbackEXT == nullptr || vkCreateDebugReportCallbackEXT(vkiInstance, &ciCallback, nullptr, &clbkValidation) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up the validation layer debug callback");
        }
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
    // retreive the handle to the presentation
    vkGetDeviceQueue(vkdevLogicalDevice, iPresentationQueueFamily, 0, &qPresentationQueue);
}


// Load shader code and create the module.
VkShaderModule GfxAPIVulkan::CreateShaderModule(const std::string &strFilename) {
    // NOTE: This needs to be recoded to support relative paths and proper resource management
    auto achShaderCode = LoadShader(strFilename);

    // describe the shader module
    VkShaderModuleCreateInfo ciShaderModule = {};
    ciShaderModule.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // bind the shader binary code
    ciShaderModule.codeSize = achShaderCode.size();
    ciShaderModule.pCode = reinterpret_cast<const uint32_t*> (achShaderCode.data());

    // createh the shader module
    VkShaderModule modShaderModule;
    if (vkCreateShaderModule(vkdevLogicalDevice, &ciShaderModule, nullptr, &modShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a shader module");
    }

    return modShaderModule;
}

// Load shader bytecode from a file.
std::vector<char> GfxAPIVulkan::LoadShader(const std::string &filename) {
    // open the file and position at the end
    std::ifstream fsFile(filename, std::ios::ate | std::ios::binary);

    // if the file failed to open, throw an error
    if (!fsFile.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    // get the file size and preallocate the read buffer
    size_t ctFileSize = fsFile.tellg();
    std::vector<char> achReadBuffer(ctFileSize);

    // rewind to the beginning and read the content into the buffer
    fsFile.seekg(0);
    fsFile.read(achReadBuffer.data(), ctFileSize);

    // close the file
    fsFile.close();

    return achReadBuffer;
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

    // describe the subpass dependency - making sure that the subpass doesn't begin before an buffer is available
    VkSubpassDependency infDependency = {};
    // the subpass waited on is the implicit subpass that usually happens at the start of the pipeline
    infDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // the subpass needs to wait until the swap chain is finished reading from the buffer (presenting the previous frame)
    infDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    infDependency.srcAccessMask = 0;
    // the dependant subpass is the apps subpass
    infDependency.dstSubpass = 0;
    // the operations that should wait are reading and writing of the color buffer
    infDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    infDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // description of the render pass to create
	VkRenderPassCreateInfo ciRenderPass = {};
	ciRenderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	// bind the color attachment
	ciRenderPass.attachmentCount = 1;
	ciRenderPass.pAttachments = &descColorAttachment;
	// bind the subpass
	ciRenderPass.subpassCount = 1;
	ciRenderPass.pSubpasses = &descSubPass;
    // bind the dependency
    ciRenderPass.dependencyCount = 0;
    ciRenderPass.pDependencies = &infDependency;

	// finally, create the render pass
	if (vkCreateRenderPass(vkdevLogicalDevice, &ciRenderPass, nullptr, &vkpassRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the render pass");
	}

}


// Create descriptor sets - used to bind uniforms to shaders.
void GfxAPIVulkan::CreateDescriptorSetLayout() {
    // describe the descriptor set binding
    VkDescriptorSetLayoutBinding infoDescriptorSetBinding = {};
    // set the binding index (defined in the shader)
    infoDescriptorSetBinding.binding = 0;
    // this describes a uniform buffer
    infoDescriptorSetBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // it contains a single uniform buffer object
    infoDescriptorSetBinding.descriptorCount = 1;
    // the descriptor set is meant for the vertex program
    infoDescriptorSetBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // describe the descriptor set layout
    VkDescriptorSetLayoutCreateInfo infoDescriptorSetLayout = {};
    infoDescriptorSetLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // set the binding description
    infoDescriptorSetLayout.bindingCount = 1;
    infoDescriptorSetLayout.pBindings = &infoDescriptorSetBinding;

    // create the layout
    if (vkCreateDescriptorSetLayout(vkdevLogicalDevice, &infoDescriptorSetLayout, nullptr, &vkhDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create the descriptor set layout");
    }
}

// Create the graphics pipeline.
void GfxAPIVulkan::CreateGraphicsPipeline() {

    // load the vertex module
    VkShaderModule modVert = CreateShaderModule("d:/Work/VulcanTutorial/Shaders/vert.spv");
    // describe the vertex shader stage
    VkPipelineShaderStageCreateInfo ciShaderStageVert = {};
    ciShaderStageVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // this is the vertex shader stage
    ciShaderStageVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // bind the vertex module
    ciShaderStageVert.pName = "main";
    ciShaderStageVert.module = modVert;

    // load the fragment module
    VkShaderModule modFrag = CreateShaderModule("d:/Work/VulcanTutorial/Shaders/frag.spv");
    // describe the fragment shader stage
    VkPipelineShaderStageCreateInfo ciShaderStageFrag = {};
    ciShaderStageFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // this is the fragment shader stage
    ciShaderStageFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // bind the vertex module
    ciShaderStageFrag.pName = "main";
    ciShaderStageFrag.module = modFrag;

    // create the array of shader stages to bind to the pipeline
    VkPipelineShaderStageCreateInfo aciShaderStages[] = { ciShaderStageVert, ciShaderStageFrag };

    // describe the vertex program inputs
	VkPipelineVertexInputStateCreateInfo ciVertexInput = {};
	ciVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// bind the binding descriptions
    auto descBinding = Vertex::GetBindingDescription();
	ciVertexInput.vertexBindingDescriptionCount = 1;
	ciVertexInput.pVertexBindingDescriptions = &descBinding;
	// bind the vertex attributes
    auto adescAttributes = Vertex::GetAttributeDescriptions();
	ciVertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(adescAttributes.size());
	ciVertexInput.pVertexAttributeDescriptions = adescAttributes.data();

	// describe the topology and if primitive restart will be used
	VkPipelineInputAssemblyStateCreateInfo ciInputAssembly;
    ciInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// triangle list will be used
	ciInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// no primitive restart (if this is set to TRUE, index of 0xFFFF/0xFFFFFFFF means that the next index starts a new primitive)
	ciInputAssembly.primitiveRestartEnable = VK_FALSE;
    ciInputAssembly.flags = 0;

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
    ciRasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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


	// describe the graphics pipeline layout
	VkPipelineLayoutCreateInfo ciPipelineLayout = {};
	ciPipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // bind the descriptor set layout
	ciPipelineLayout.setLayoutCount = 1;
	ciPipelineLayout.pSetLayouts = &vkhDescriptorSetLayout;
    // not using push constants at the moment
	ciPipelineLayout.pushConstantRangeCount = 0;
	ciPipelineLayout.pPushConstantRanges = 0;

	// create the pipeline
	if (vkCreatePipelineLayout(vkdevLogicalDevice, &ciPipelineLayout, nullptr, &vkplPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the pipeline layout!");
	}

    
    // finally, describe the graphics pipeline itself
    VkGraphicsPipelineCreateInfo ciGraphicsPipeline = {};
    ciGraphicsPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // bind the shader stages
    ciGraphicsPipeline.stageCount = 2;
    ciGraphicsPipeline.pStages = aciShaderStages;
    // bind the rest of prepared configurations
    ciGraphicsPipeline.pVertexInputState = &ciVertexInput;
    ciGraphicsPipeline.pInputAssemblyState = &ciInputAssembly;
    ciGraphicsPipeline.pViewportState = &ciViewportState;
    ciGraphicsPipeline.pRasterizationState = &ciRasterizationState;
    ciGraphicsPipeline.pMultisampleState = &ciMultisampling;
    ciGraphicsPipeline.pDepthStencilState = nullptr;
    ciGraphicsPipeline.pColorBlendState = &ciColorBlendState;
    ciGraphicsPipeline.pDynamicState = nullptr;
    // set the pipeline layout
    ciGraphicsPipeline.layout = vkplPipelineLayout;
    // set up the render pass
    ciGraphicsPipeline.renderPass = vkpassRenderPass;
    ciGraphicsPipeline.subpass = 0;
    // this pipeline doesn't derive from another pipeline (could be done as an optimization)
    ciGraphicsPipeline.basePipelineHandle = VK_NULL_HANDLE;
    ciGraphicsPipeline.basePipelineIndex = -1;

    // create the graphics pipeline
    if (vkCreateGraphicsPipelines(vkdevLogicalDevice, VK_NULL_HANDLE, 1, &ciGraphicsPipeline, nullptr, &vkgpipePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the graphics pipeline");
    }

    // destroy shader modules - they are a part of the graphics pipeline
    vkDestroyShaderModule(vkdevLogicalDevice, modFrag, nullptr);
    vkDestroyShaderModule(vkdevLogicalDevice, modVert, nullptr);
}


// Create the framebuffers.
void GfxAPIVulkan::CreateFramebuffers() {
    // resize the frame buffer array to match the number of swap chain image views
    atgtFramebuffers.resize(aimgvImageViews.size());

    // prepare the common part of the framebuffer description
    VkFramebufferCreateInfo ciFramebuffer = {};
    ciFramebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    // bind the render pass
    ciFramebuffer.renderPass = vkpassRenderPass;
    // set the extends for the frame buffer
    ciFramebuffer.width = sexExtent.width;
    ciFramebuffer.height = sexExtent.height;
    // only one layer
    ciFramebuffer.layers = 1;
    // there will only be one image view
    ciFramebuffer.attachmentCount = 1;

    // create a frame buffer for each image view
    for (int iImageView = 0; iImageView < aimgvImageViews.size(); iImageView++) {
        // create the image view attachment
        VkImageView aimgvAttachments[] = {
            aimgvImageViews[iImageView]
        };

        // bind the image view to the framebuffer
        ciFramebuffer.pAttachments = aimgvAttachments;

        // create the framebuffer
        if (vkCreateFramebuffer(vkdevLogicalDevice, &ciFramebuffer, nullptr, &atgtFramebuffers[iImageView]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a framebuffer");
        }
    }
}

// Destroy the framebuffers.
void GfxAPIVulkan::DestroyFramebuffers() {
    for (VkFramebuffer tgtFramebuffer : atgtFramebuffers) {
        vkDestroyFramebuffer(vkdevLogicalDevice, tgtFramebuffer, nullptr);
    }
}


// Create the command pool.
void GfxAPIVulkan::CreateCommandPool() {
    // describe the command pool
    VkCommandPoolCreateInfo ciCommandPool = {};
    ciCommandPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // bind the graphics queue family to the command pool
    ciCommandPool.queueFamilyIndex = iGraphicsQueueFamily;
    // clear all flags
    ciCommandPool.flags = 0;

    // create the command pool
    if (vkCreateCommandPool(vkdevLogicalDevice, &ciCommandPool, nullptr, &vkhCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the command pool");
    }
}

// Create the command buffers.
void GfxAPIVulkan::CreateCommandBuffers() {
    // one command buffer is needed per framebuffer
    acbufCommandBuffers.resize(atgtFramebuffers.size());

    // describe the allocation of command buffers - all will be allocated with one call
    VkCommandBufferAllocateInfo ciAllocateBuffers;
    ciAllocateBuffers.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // bind the command pool
    ciAllocateBuffers.commandPool = vkhCommandPool;
    // these are rimary buffers - can be directly submitted for execution
    ciAllocateBuffers.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // set the number of buffers
    ciAllocateBuffers.commandBufferCount = (uint32_t) acbufCommandBuffers.size();

    // allocate the command buffers
    if (vkAllocateCommandBuffers(vkdevLogicalDevice, &ciAllocateBuffers, acbufCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create allocate command buffers");
    }
}


// Record the command buffers - NOTE: this is for the simple drawing from the tutorial.
void GfxAPIVulkan::RecordCommandBuffers() {
    //  describe how the command buffers will be used
    VkCommandBufferBeginInfo ciCommandBufferBegin = {};
    ciCommandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // it is possible that the command buffer will be resubmitted before the prebious submission has finished executing
    ciCommandBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    // primary command buffers don't inherit from anything
    ciCommandBufferBegin.pInheritanceInfo = nullptr;

    // define the fraembuffer clear color as black
    VkClearValue colClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    // describe how the render pass will be used
    VkRenderPassBeginInfo ciRenderPassBegin = {};
    ciRenderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // bind the render pass definition
    ciRenderPassBegin.renderPass = vkpassRenderPass;
    // set the render area
    ciRenderPassBegin.renderArea.offset = { 0,0 };
    ciRenderPassBegin.renderArea.extent = sexExtent;
    // set the clear color
    ciRenderPassBegin.clearValueCount = 1;
    ciRenderPassBegin.pClearValues = &colClearColor;

    // record the same commands in all buffers
    for (int iCommandBuffer = 0; iCommandBuffer < acbufCommandBuffers.size(); iCommandBuffer++) {
        VkCommandBuffer &cbufCommandBuffer = acbufCommandBuffers[iCommandBuffer];
        // begin the command buffer
        vkBeginCommandBuffer(cbufCommandBuffer, &ciCommandBufferBegin);

        // bind the frame buffer to the render pass
        ciRenderPassBegin.framebuffer = atgtFramebuffers[iCommandBuffer];

        // issue (record) the command to begin the render pass, with the command executed from the primary buffer
        vkCmdBeginRenderPass(cbufCommandBuffer, &ciRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        // issue the command to bind the graphics pipeline
        vkCmdBindPipeline(cbufCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkgpipePipeline);

        // bind the vertex buffer
        VkBuffer avkhBuffers[] = { vkhVertexBuffer };
        VkDeviceSize actOffsets[] = { 0 };
        vkCmdBindVertexBuffers(cbufCommandBuffer, 0, 1, avkhBuffers, actOffsets);
        // bind the index buffer
        vkCmdBindIndexBuffer(cbufCommandBuffer, vkhIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // issue the draw command to draw index buffers
        vkCmdDrawIndexed(cbufCommandBuffer, static_cast<uint32_t>(aiIndices.size()), 1, 0, 0, 0);

        // issue the command to end the render pass
        vkCmdEndRenderPass(cbufCommandBuffer);

        // end the command buffer
        if (vkEndCommandBuffer(cbufCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer");
        }
    }
}

// Create semaphores for syncing buffer and renderer access.
void GfxAPIVulkan::CreateSemaphores() {
    
    // describe the semaphores
    VkSemaphoreCreateInfo ciSemaphore = {};
    ciSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // cerate the semaphores
    if (vkCreateSemaphore(vkdevLogicalDevice, &ciSemaphore, nullptr, &syncImageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(vkdevLogicalDevice, &ciSemaphore, nullptr, &syncRender) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create semaphores");
    }
}

// Delete the semaphores.
void GfxAPIVulkan::DestroySemaphores() {
    vkDestroySemaphore(vkdevLogicalDevice, syncImageAvailable, nullptr);
    vkDestroySemaphore(vkdevLogicalDevice, syncRender, nullptr);
}


// Create vertex buffers.
void GfxAPIVulkan::CreateVertexBuffers() {
    // create the vertex buffer
    VkDeviceSize ctBufferSize = sizeof(avVertices[0]) * avVertices.size();

    // create a staging buffer - it is a source in a memory transfer operation, and is located on the host
    VkBuffer vkhStagingBuffer;
    VkDeviceMemory vkhStagingMemory;
    CreateBuffer(ctBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkhStagingBuffer, vkhStagingMemory);
    
    // to copy the vertex buffer values to GPU memory, it first needs to be mapped to CPU
    void *pMappedMemory;
    vkMapMemory(vkdevLogicalDevice, vkhStagingMemory, 0, ctBufferSize, 0, &pMappedMemory);
    // copy the buffer to mapped memory
    memcpy(pMappedMemory, avVertices.data(), ctBufferSize);
    // unmap memory, let the GPU take over
    vkUnmapMemory(vkdevLogicalDevice, vkhStagingMemory);

    // create the vertex buffer - it is located in device memory and is a memory transfer destination
    CreateBuffer(ctBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkhVertexBuffer, vkhVertexBufferMemory);

    // copy staging buffer contents to the vertex buffer
    CopyBuffer(vkhStagingBuffer, vkhVertexBuffer, ctBufferSize);

    // destroy the staging buffer
    vkDestroyBuffer(vkdevLogicalDevice, vkhStagingBuffer, nullptr);
    // free buffer memory
    vkFreeMemory(vkdevLogicalDevice, vkhStagingMemory, nullptr);
}


// Create index buffer.
void GfxAPIVulkan::CreateIndexBuffers() {
    // get the index buffer size
    VkDeviceSize ctBufferSize = sizeof(aiIndices[0]) * aiIndices.size();

    // create a staging buffer - it is a source in a memory transfer operation, and is located on the host
    VkBuffer vkhStagingBuffer;
    VkDeviceMemory vkhStagingMemory;
    CreateBuffer(ctBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkhStagingBuffer, vkhStagingMemory);

    // to copy the index buffer values to GPU memory, it first needs to be mapped to CPU
    void *pMappedMemory;
    vkMapMemory(vkdevLogicalDevice, vkhStagingMemory, 0, ctBufferSize, 0, &pMappedMemory);
    // copy the buffer to mapped memory
    memcpy(pMappedMemory, aiIndices.data(), ctBufferSize);
    // unmap memory, let the GPU take over
    vkUnmapMemory(vkdevLogicalDevice, vkhStagingMemory);

    // create the index buffer - it is located in device memory and is a memory transfer destination
    CreateBuffer(ctBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkhIndexBuffer, vkhIndexBufferMemory);

    // copy staging buffer contents to the index buffer
    CopyBuffer(vkhStagingBuffer, vkhIndexBuffer, ctBufferSize);

    // destroy the staging buffer
    vkDestroyBuffer(vkdevLogicalDevice, vkhStagingBuffer, nullptr);
    // free buffer memory
    vkFreeMemory(vkdevLogicalDevice, vkhStagingMemory, nullptr);
}


// Create a buffer - vertex, transfer, index...
void GfxAPIVulkan::CreateBuffer(VkDeviceSize ctSize, VkBufferUsageFlags flgBufferUsage, VkMemoryPropertyFlags flgMemoryProperties, VkBuffer &vkhBuffer, VkDeviceMemory &vkhMemory) {
    // describe the vertex buffer
    VkBufferCreateInfo infoBuffer = {};
    infoBuffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // set the size in bytes
    infoBuffer.size = ctSize;
    // mark that this describes a vertex buffer
    infoBuffer.usage = flgBufferUsage;
    // mark that the buffer is excluseive to one queue and not shared between multiple queues
    infoBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // create the vertex buffer
    if (vkCreateBuffer(vkdevLogicalDevice, &infoBuffer, nullptr, &vkhBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the vertex buffer");
    }

    // get the buffer's memory requirements
    VkMemoryRequirements propsMemoryRequirements = {};
    vkGetBufferMemoryRequirements(vkdevLogicalDevice, vkhBuffer, &propsMemoryRequirements);

    // describe the memory allocation
    VkMemoryAllocateInfo infoBufferMemory = {};
    infoBufferMemory.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    // how much memory to allocate
    infoBufferMemory.allocationSize = propsMemoryRequirements.size;
    // find the appropriate memory type
    infoBufferMemory.memoryTypeIndex = FindMemoryType(propsMemoryRequirements.memoryTypeBits, flgMemoryProperties);

    // allocate the memory for the buffer
    if (vkAllocateMemory(vkdevLogicalDevice, &infoBufferMemory, nullptr, &vkhMemory) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate memory for the vertex buffer");
    }

    // after a successfull allocation, bind the memory to the buffer
    vkBindBufferMemory(vkdevLogicalDevice, vkhBuffer, vkhMemory, 0);
}


// Copy memory from one buffer to the other.
void GfxAPIVulkan::CopyBuffer(VkBuffer vkhSourceBuffer, VkBuffer vkhDestinationBuffer, VkDeviceSize ctSize) {
    // create a temporary command buffer
    VkCommandBufferAllocateInfo infoCommandBuffer = {};
    infoCommandBuffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // it is a primary buffer
    infoCommandBuffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // it uses the same command pool - NOTE: it would be more optimal to create a new command pool for temp buffers
    infoCommandBuffer.commandPool = vkhCommandPool;
    // only one buffer will be allocated
    infoCommandBuffer.commandBufferCount = 1;

    // allocate teh buffer
    VkCommandBuffer vkhCommandBuffer = {};
    vkAllocateCommandBuffers(vkdevLogicalDevice, &infoCommandBuffer, &vkhCommandBuffer);

    // start recording the command buffer
    VkCommandBufferBeginInfo infoBegin = {};
    infoBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // this buffer is only going to be submitted once
    infoBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // start recording
    vkBeginCommandBuffer(vkhCommandBuffer, &infoBegin);

    // create the copy command - copies start from beggining, size is the size specified in the input arguments
    VkBufferCopy cmdCopy = {};
    cmdCopy.srcOffset = 0;
    cmdCopy.dstOffset = 0;
    cmdCopy.size = ctSize;

    // run the copy command
    vkCmdCopyBuffer(vkhCommandBuffer, vkhSourceBuffer, vkhDestinationBuffer, 1, &cmdCopy);

    // stop recording the buffer
    vkEndCommandBuffer(vkhCommandBuffer);

    // prepare the command buffer submit info for the copy operation
    VkSubmitInfo infoSubmitCopy = {};
    infoSubmitCopy.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // set the buffer to execute
    infoSubmitCopy.commandBufferCount = 1;
    infoSubmitCopy.pCommandBuffers = &vkhCommandBuffer;

    // submit the queue for execution
    vkQueueSubmit(qGraphicsQueue, 1, &infoSubmitCopy, VK_NULL_HANDLE);
    // wait for the commands to finish
    vkQueueWaitIdle(qGraphicsQueue);

    // clean up the command buffer
    vkFreeCommandBuffers(vkdevLogicalDevice, vkhCommandPool, 1, &vkhCommandBuffer);
}


// Get the graphics memory type with the desired properties.
uint32_t GfxAPIVulkan::FindMemoryType(uint32_t flgTypeFilter, VkMemoryPropertyFlags flgProperties) {
    // get all memory types for the physical device
    VkPhysicalDeviceMemoryProperties propsDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vkdevPhysicalDevice, &propsDeviceMemoryProperties);

    // go through all memory types an find the suitable one
    for (uint32_t iMemoryType = 0; iMemoryType < propsDeviceMemoryProperties.memoryTypeCount; iMemoryType++) {
        // if the type index matches the filter and memory type propeties match the requested ones
        if ((flgTypeFilter & (1 << iMemoryType)) && (propsDeviceMemoryProperties.memoryTypes[iMemoryType].propertyFlags & flgProperties) == flgProperties) {
            // return the memory type index
            return iMemoryType;
        }
    }
    // if the appropriate memory type wasn't found, throw an exception
    throw std::runtime_error("Unable to find an appropriate memory type");
}


// Called when the application's window is resized.
void GfxAPIVulkan::OnWindowResized(GLFWwindow* window, uint32_t width, uint32_t height) {
    // have the window update its dimensions
    _wndWindow->UpdateDimensions();
    // swap chain needs to be recreated to be able to render again
    InitializeSwapChain();
}

// Render a frame.
void GfxAPIVulkan::Render() {
    // obtain a target image from the swap chain
    // setting max uint64 as the timeout (in nanoseconds) disables the timeout
    // when the image becomes available the syncImageAvailable semaphore will be signaled
    uint32_t iImage;
    VkResult statusResult  = vkAcquireNextImageKHR(vkdevLogicalDevice, swcSwapChain, std::numeric_limits<uint64_t>::max(), syncImageAvailable, VK_NULL_HANDLE, &iImage);

    // if acquiring the image failed because the swap chain has become incompatible with the surface
    if (statusResult == VK_ERROR_OUT_OF_DATE_KHR) {
        // setup the swap chain for the current surface
        InitializeSwapChain();
    // else, if the operation failed with no way to recover
    } else if (statusResult != VK_SUCCESS && statusResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }
    // note that we consider suboptimal surface as success - this is something that could be handled better/differently by, for example, recreating the swap chain

    // describe how the queue will be submitted and synchronized
    VkSubmitInfo infSubmit = {};
    infSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // bind the image semaphore that the queue has to wait on before it starts executing
    VkSemaphore asyncWait[] = { syncImageAvailable };
    infSubmit.waitSemaphoreCount = 1;
    infSubmit.pWaitSemaphores = asyncWait;

    // at what stage of the pipeline should the queue wait for the semaphore
    // this sets the stage to the fragment program, making it possible for the vertex program to run before waiting
    VkPipelineStageFlags aflgWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    infSubmit.pWaitDstStageMask = aflgWaitStages;

    // bind the command buffer
    infSubmit.commandBufferCount = 1;
    infSubmit.pCommandBuffers = &acbufCommandBuffers[iImage];

    // set the semaphores that will be signalled when the command buffers are executed
    VkSemaphore asyncSignal[] = { syncRender };
    infSubmit.signalSemaphoreCount = 1;
    infSubmit.pSignalSemaphores = asyncSignal;

    // submit the command buffers to the queue
    if (vkQueueSubmit(qGraphicsQueue, 1, &infSubmit, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    // describe how to present the image
    VkPresentInfoKHR infPresent = {};
    infPresent.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // presentation should wait for the render semaphore to be signalled
    infPresent.waitSemaphoreCount = 1;
    infPresent.pWaitSemaphores = asyncSignal;

    // what images to present to which swap chains
    VkSwapchainKHR aswcChains[] = { swcSwapChain };
    infPresent.swapchainCount = 1;
    infPresent.pSwapchains = aswcChains;
    infPresent.pImageIndices = &iImage;

    // where to store the results (success/fail) of presentation, per swap chain
    // not needed for a single swap chain, result of the presentation function can be used
    infPresent.pResults = nullptr;

    // present the queue
    statusResult = vkQueuePresentKHR(qPresentationQueue, &infPresent);

    // if presentation failed because the swap chain has become incompatible with the surface
    if (statusResult == VK_ERROR_OUT_OF_DATE_KHR) {
        // setup the swap chain for the current surface
        InitializeSwapChain();
    // else, if the operation failed with no way to recover
    } else if (statusResult != VK_SUCCESS && statusResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to present swap chain image");
    }
    // note that we consider suboptimal surface as success - this is something that could be handled better/differently by, for example, recreating the swap chain

    // wait for the device to finish rendering
    // not needed in a proper application where there are other things to do while the grahics card and thread to their thing
    vkDeviceWaitIdle(vkdevLogicalDevice);
}
