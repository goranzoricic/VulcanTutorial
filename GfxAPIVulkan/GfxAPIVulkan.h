#pragma once
#include "../GfxAPI/GfxAPI.h"
#include <vulkan/vulkan.h>

struct GLFWwindow;

// Implementation of Vulkan graphics API.
class GfxAPIVulkan : public GfxAPI {
public:
    static void GfxAPIVulkan::OnWindowResizedCallback(GLFWwindow* window, int width, int height);

private:
    GfxAPIVulkan() :vkdevPhysicalDevice(VK_NULL_HANDLE), 
                    vkdevLogicalDevice(VK_NULL_HANDLE), 
                    iGraphicsQueueFamily(-1),
                    qGraphicsQueue(VK_NULL_HANDLE),
                    iPresentationQueueFamily(-1),
                    qPresentationQueue(VK_NULL_HANDLE),
                    swcSwapChain(VK_NULL_HANDLE),
                    vkpassRenderPass(VK_NULL_HANDLE),
                    vkplPipelineLayout(VK_NULL_HANDLE),
                    vkgpipePipeline(VK_NULL_HANDLE)
    {};
    ~GfxAPIVulkan() {};
    friend class GfxAPI;

public:
    // Initialize the API. Returns true if successfull.
    virtual bool Initialize(uint32_t dimWidth, uint32_t dimHeight);
    // Destroy the API. Returns true if successfull.
    virtual bool Destroy();

    // Render a frame.
    virtual void Render(); 

private:
    // Called when the application's window is resized.
    void OnWindowResized(GLFWwindow* window, uint32_t width, uint32_t height);

private:
    // Initialize the application window.
    void CreateWindow(uint32_t dimWidth, uint32_t dimHeight);
    // Create the Vulkan instance.
    void CreateInstance();

    // Initialize swap chain. Called on first initialization, but also on window resize.
    void InitializeSwapChain();
    // Destroy the swap chain.
    void DestroySwapChain();

    // Get the Vulkan instance extensions required for the applciation to work.
    void GetRequiredInstanceExtensions(std::vector<const char*> &astrRequiredExtensions) const;
    // Check if all required instance extensions are supported.
    void CheckInstanceExtensionSupport(const std::vector<const char*> &astrRequiredExtensions) const;
    // Get the Vulkan device extensions required for the applciation to work.
    void GetRequiredDeviceExtensions(std::vector<const char*> &astrRequiredExtensions) const;
    // Check if all required device extensions are supported.
    void CheckDeviceExtensionSupport(const VkPhysicalDevice &device, const std::vector<const char*> &astrRequiredExtensions) const;

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
    bool IsDeviceSuitable(const VkPhysicalDevice &vkdevDevice);

    // Find indices of queue families needed to support all application's features.
    void FindQueueFamilies(const VkPhysicalDevice &device);
    // Do the queue families support all required features?
    bool IsQueueFamiliesSuitable() const;

    // Collect information about swap chain feature support.
    void QuerySwapChainSupport(const VkPhysicalDevice &device);
    // Create the swap chain to use for presenting images.
    void CreateSwapChain();
    // Select the swap chain format to use.
    void SelectSwapChainFormat();
    // Select the presentation mode to use.
    void SelectSwapChainPresentMode();
    // Select the swap chain extent to use.
    void SelectSwapChainExtent();

    // Create the logical device the application will use. Also creates the queues that commands will be submitted to.
    void CreateLogicalDevice();

    // Create the image views needed to acces swap chain images.
    void CreateImageViews();
    // Destroy the image views.
    void DestroyImageViews();

    // Load shaders and create shader modules.
    VkShaderModule CreateShaderModule(const std::string &strFilename);
    // Load shader bytecode from a file.
    std::vector<char> LoadShader(const std::string &filename);

    // Create the render pass.
	void CreateRenderPass();
	// Create the graphics pipeline.
	void CreateGraphicsPipeline();

    // Create the framebuffers.
    void CreateFramebuffers();
    // Destroy the framebuffers.
    void DestroyFramebuffers();

    // Create the command pool.
    void CreateCommandPool();
    // Create the command buffers.
    void CreateCommandBuffers();

    // Record the command buffers - NOTE: this is for the simple drawing from the tutorial.
    void RecordCommandBuffers();

    // Create semaphores for syncing buffer and renderer access.
    void CreateSemaphores();
    // Delete the semaphores.
    void DestroySemaphores();

private:
    // Handle to the vulkan instance.
    VkInstance vkiInstance;

    // Handle to the window surface that the render buffers will be presented to.
    VkSurfaceKHR sfcSurface;
    // Capabilities of the drawing surface.
    VkSurfaceCapabilitiesKHR capsSurface;

    // Swap chain to use for rendering.
    VkSwapchainKHR swcSwapChain;
    // Drawing formats that the device support.
    std::vector<VkSurfaceFormatKHR> aFormats;
    // Present modes supported by the surface.
    std::vector<VkPresentModeKHR> aPresentModes;
    // Handles to swap chain images.
    std::vector<VkImage> aimgImages;
    // Views to swap chain images.
    std::vector<VkImageView> aimgvImageViews;

    // Swap chain format selected for use.
    VkSurfaceFormatKHR sfmtFormat;
    // Present mode selected for use
    VkPresentModeKHR spmPresentMode;
    // Extent (resolution) selected for the swap chain.
    VkExtent2D sexExtent;

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

    // Index of a graphics family with presentation support.
    int iPresentationQueueFamily;
    // Handle to the queue to use for presentation.
    VkQueue qPresentationQueue;

	// Render pass applied to render objects.
	VkRenderPass vkpassRenderPass;
	// Layout of the graphics pipeline.
	VkPipelineLayout vkplPipelineLayout;
    // Graphics pipeline.
    VkPipeline vkgpipePipeline;

    // Framebuffers used to draw.
    std::vector<VkFramebuffer> atgtFramebuffers;

    // Command pool that will hold command buffers.
    VkCommandPool vkhCommandPool;
    // Command buffers to post the commands to.
    std::vector<VkCommandBuffer> acbufCommandBuffers;

    // Semephore used to sync target buffers.
    VkSemaphore syncImageAvailable;
    // Semaphore used to sync presentation.
    VkSemaphore syncRender;
};

