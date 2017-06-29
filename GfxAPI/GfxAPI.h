#pragma once

enum GfxAPIType {
    GFX_API_TYPE_INVALID = -1,
    GFX_API_TYPE_NULL = 0,
    GFX_API_TYPE_VULKAN = 1,
};

class Window;

// This is a base class for graphics APIs. It defines the interface that an API needs to provide
// for the application and the render. The class is abstract, all required methods need to be implemented
// by concrete classes.
class GfxAPI {
public:
    // Get the current graphics API instance.
    static GfxAPI *Get();

    // Create a Vulkan graphics API.
    static GfxAPI *CreateVulkan();
    // Create a Null graphics API.
    static GfxAPI *CreateNull();
    
public:
    // Initialize the API. Returns true if successfull. Pass window dimensions.
    virtual bool Initialize(uint32_t dimWidth, uint32_t dimHeight) = 0;
    // Destroy the API. Returns true if successfull.
    virtual bool Destroy() = 0;
    // Get the main application window.
    std::shared_ptr<Window> &GetWindow() { return _wndWindow;  }

protected:
    // Constructor and destructor are only available to derived classes.
    GfxAPI() {};
    virtual ~GfxAPI() {};

public:
    // Forbid the copy constructor and assignment to prevent multiple copies.
    GfxAPI(GfxAPI const &) = delete;
    void operator = (GfxAPI const &) = delete;

protected:
    // Application window
    std::shared_ptr<Window> _wndWindow;

private:
    // The currently active graphics API implementation. There can be only one.
    static GfxAPI *_apiInstance;

};

