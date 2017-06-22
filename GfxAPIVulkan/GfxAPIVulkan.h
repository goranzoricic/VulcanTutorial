#pragma once
#include "../GfxAPI/GfxAPI.h"
#include <vulkan/vulkan.h>

// Implementation of Vulkan graphics API.
class GfxAPIVulkan : public GfxAPI {
private:
    GfxAPIVulkan() : vkdevPhysicalDevice(VK_NULL_HANDLE) {};
    ~GfxAPIVulkan() {};
    friend class GfxAPI;

public:
    // Initialize the API. Returns true if successfull.
    virtual bool Initialize(uint32_t dimWidth, uint32_t dimHeight);
    // Destroy the API. Returns true if successfull.
    virtual bool Destroy();

private:
    // Initialize the application window
    void CreateWindow(uint32_t dimWidth, uint32_t dimHeight);
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
    bool IsDeviceSuitable(const VkPhysicalDevice &vkdevDevice) const;

private:
    // Handle to the vulkan instance
    VkInstance vkiInstance;
    // Handle to the debug callback
    VkDebugReportCallbackEXT clbkValidation;
    // Physical device (graphics card) used
    VkPhysicalDevice vkdevPhysicalDevice;
};

