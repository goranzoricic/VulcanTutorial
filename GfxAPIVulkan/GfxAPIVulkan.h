#pragma once
#include "../GfxAPI/GfxAPI.h"
#include <vulkan/vulkan.h>

struct GLFWwindow;

// Implementation of Vulkan graphics API.
class GfxAPIVulkan : public GfxAPI {
private:
    // NOTE: refactor this
    struct Vertex {
        glm::vec3 vecPosition;
        glm::vec3 colColor;
        glm::vec2 vecTexCoords;

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
        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> adescAttributes = {};
            // set up the description of the vertex position
            // data comes from the binding 0 (set up above)
            adescAttributes[0].binding = 0;
            // data goes to the location 0 (specified in the vertex shader)
            adescAttributes[0].location = 0;
            // data is two 32bit floats (screen x, y)
            adescAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
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

            // set up the description of the texture coordinates
            // data comes from the binding 0 (set up above)
            adescAttributes[2].binding = 0;
            // data goes to the location 0 (specified in the vertex shader)
            adescAttributes[2].location = 2;
            // data is three 32bit floats (red, green, blue)
            adescAttributes[2].format = VK_FORMAT_R32G32_SFLOAT;
            // offset of this attribute from the start of the data block
            adescAttributes[2].offset = offsetof(Vertex, vecTexCoords);

            return adescAttributes;
        };
    };
    std::vector<Vertex> avVertices;
    std::vector<uint32_t> aiIndices;

private:
    // Uniform buffer description.
    struct UniformBufferObject {
        // Model transform.
        glm::mat4 tModel;
        // View transform.
        glm::mat4 tView;
        // Projection transform.
        glm::mat4 tProjection;
    };

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
                    vkhDescriptorSetLayout(VK_NULL_HANDLE),
                    vkhDescriptorPool(VK_NULL_HANDLE),
                    vkplPipelineLayout(VK_NULL_HANDLE),
                    vkgpipePipeline(VK_NULL_HANDLE),
                    vkhVertexBuffer(VK_NULL_HANDLE),
                    vkhVertexBufferMemory(VK_NULL_HANDLE),
                    vkhUniformBuffer(VK_NULL_HANDLE),
                    vkhUniformBufferMemory(VK_NULL_HANDLE),
                    vkhImageData(VK_NULL_HANDLE),
                    vkhImageMemory(VK_NULL_HANDLE),
                    vkhImageView(VK_NULL_HANDLE),
                    vkhImageSampler(VK_NULL_HANDLE)
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

    // Update the uniform buffer - MVP matrices.
    // The tutorial implementation rotates the object 90 degrees per second.
    void UpdateUniformBuffer();

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
    // Create descriptor sets - used to bind uniforms to shaders.
    void CreateDescriptorSetLayout();
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

    // Create resources needed for depth testing.
    void CreateDepthResources();

    // Create a texture.
    void CreateTextureImage();
    // Create a view for the texture.
    void CreateTextureImageVeiw();
    // Create a sampler for the texture.
    void CreateImageSampler();

    // Find the format to use for depth.
    VkFormat FindDepthFormat();
    // Find the first supported format from a list of formats.
    VkFormat FindSupportedFormat(const std::vector<VkFormat> &afmtFormats, VkImageTiling imtTiling, VkFormatFeatureFlags flagFormatFeatures);
    // Does the format have the stencil component
    bool FormatHasStencilComponent(VkFormat fmtFormat);

    // Create an image view
    VkImageView CreateImageView(VkImage vkhImage, VkFormat fmtFormat, VkImageAspectFlags flagImageAspect);
    // Create an image.
    void CreateImage(uint32_t dimWidth, uint32_t dimHeight, VkFormat fmtFormat, VkImageTiling imtTiling, VkImageUsageFlags flagUsage, VkMemoryPropertyFlags flagMemoryProperties, VkImage &vkhImage, VkDeviceMemory &vkhMemory);
    // Change image layout to what is needed for rendering.
    void TransitionImageLayout(VkImage vkhImage, VkFormat fmtFormat, VkImageLayout imlOldLayout, VkImageLayout imlNewLayout);
    // Copy a buffer to the image.
    void CoypBufferToImage(VkBuffer vkhBuffer, VkImage vkhImage, uint32_t dimWidth, uint32_t dimHeight);

    // Load the example model.
    void LoadModel();

    // Create vertex buffer.
    void CreateVertexBuffers();
    // Create index buffer.
    void CreateIndexBuffers();
    // Create uniform buffer.
    void CreateUniformBuffers();

    // Create the descriptor pool.
    void CreateDescriptorPool();
    // Create the descriptor set.
    void CreateDescriptorSet();

    // Get the graphics memory type with the desired properties.
    uint32_t FindMemoryType(uint32_t flgTypeFilter, VkMemoryPropertyFlags flgProperties);

    // Create a buffer - vertex, transfer, index...
    void CreateBuffer(VkDeviceSize ctSize, VkBufferUsageFlags flgBufferUsage, VkMemoryPropertyFlags flagMemoryProperties, VkBuffer &vkhBuffer, VkDeviceMemory &vkhMemory);
    // Copy memory from one buffer to the other.
    void CopyBuffer(VkBuffer vkhSourceBuffer, VkBuffer vkhDestinationBuffer, VkDeviceSize ctSize);
    // Start one time command recording.
    VkCommandBuffer BeginOneTimeCommand();
    // Finish one time command recording.
    void EndOneTimeCommand(VkCommandBuffer vkhCommandBuffer);

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
	
    // Descriptor set layout for uniform buffers.
    VkDescriptorSetLayout vkhDescriptorSetLayout;

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

    // Vertex buffer holding the shape's vertices.
    VkBuffer vkhVertexBuffer;
    // Memory used by the vertex buffer.
    VkDeviceMemory vkhVertexBufferMemory;

    // Image holding the texture data.
    VkImage vkhImageData;
    // Memory used by the Image buffer.
    VkDeviceMemory vkhImageMemory;
    // Image view describing how to access the image.
    VkImageView vkhImageView;
    // Sampler used in the fragment shader to read from the texture.
    VkSampler vkhImageSampler;

    // Depth image that fragment depth will be written to and tested with.
    VkImage vkhDepthImageData;
    // Memory used by the Depth image buffer.
    VkDeviceMemory vkhDepthImageMemory;
    // Depth image view describing how to access the Depth image.
    VkImageView vkhDeptImageView;

    // Index buffer holding the order of vertices in triangles.
    VkBuffer vkhIndexBuffer;
    // Memory used by the index buffer.
    VkDeviceMemory vkhIndexBufferMemory;

    // Uniform buffer holding the order of vertices in triangles.
    VkBuffer vkhUniformBuffer;
    // Memory used by the uniform buffer.
    VkDeviceMemory vkhUniformBufferMemory;

    // Descriptor pool used to allocate descriptor sets.
    VkDescriptorPool vkhDescriptorPool;
    // Descriptor set that will hold the uniform buffer.
    VkDescriptorSet vkhDescriptorSet;
};

