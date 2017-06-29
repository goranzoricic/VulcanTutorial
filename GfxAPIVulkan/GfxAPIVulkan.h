#pragma once
#include "../GfxAPI/GfxAPI.h"
#include <vulkan/vulkan.h>

// Implementation of Vulkan graphics API.
class GfxAPIVulkan : public GfxAPI {
private:
    GfxAPIVulkan() :vkdevPhysicalDevice(VK_NULL_HANDLE), 
                    vkdevLogicalDevice(VK_NULL_HANDLE), 
                    qGraphicsQueue(VK_NULL_HANDLE), 
                    iGraphicsQueueFamily(-1) 
    {};
    ~GfxAPIVulkan() {};
    friend class GfxAPI;

public:
    // Initialize the API. Returns true if successfull.
    virtual bool Initialize(uint32_t dimWidth, uint32_t dimHeight);
    // Destroy the API. Returns true if successfull.
    virtual bool Destroy();

private:
    // Initialize the application window.
    void CreateWindow(uint32_t dimWidth, uint32_t dimHeight);
    // Create the Vulkan instance.
    void CreateInstance();
    // Get the ulkan extensions required for the applciation to work.
    void GetRequiredExtensions(std::vector<const char*> &requiredExtensions);
    // Check if all required extensions are supported.
    void CheckExtensionSupport(const std::vector<const char*> &requiredExtensions);

    // NOTE: In the Vulkan SDK, Config directory, there is a vk_layer_settings.txt file that explains how to configure the validation layers.
    // Set up the validation layers.
    void SetupValidationLayers();
    // Check if all requested valdiation layers are supported.
    bool CheckValidationLayerSupport();
    // Set up the validation error callback.
    void SetupValidationErrorCallback();
    // Destroy the validation callbacks (on application end).
    void DestroyValidationErrorCallback();

    // Create the surface to present render buffers to.
    void CreateSurface();

    // Select the physical device (graphics card) to render on.
    void SelectPhysicalDevice();
    // Does the device support all required features?
    bool IsDeviceSuitable(const VkPhysicalDevice &vkdevDevice) const;
    
    // Find indices of queue families needed to support all application's features.
    void FindQueueFamilies();
    // Do the queue families support all required features?
    bool IsQueueFamiliesSuitable() const;

    // Create the logical device the application will use. Also creates the queues that commands will be submitted to.
    void CreateLogicalDevice();

private:
    // Handle to the vulkan instance.
    VkInstance vkiInstance;
    // Handle to the window surface that the render buffers will be presented to.
    VkSurfaceKHR sfcSurface;

    // Handle to the debug callback.
    VkDebugReportCallbackEXT clbkValidation;
    // Physical device (graphics card) used.
    VkPhysicalDevice vkdevPhysicalDevice;
    // Logical device used.
    VkDevice vkdevLogicalDevice;

    // Index of a queue family that supports graphics commands.
    int iGraphicsQueueFamily;
    // Handle to the queue to submit graphics commands to.
    VkQueue qGraphicsQueue;
};

