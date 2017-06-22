#pragma once
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
    
protected:
    GfxAPI() {};
    virtual ~GfxAPI() {};

public:
    // Forbid the copy constructor and assignment to prevent multiple copies.
    GfxAPI(GfxAPI const &) = delete;
    void operator = (GfxAPI const &) = delete;

private:
    // The currently active graphics API implementation. There can be only one.
    static GfxAPI *_apiInstance;
};

