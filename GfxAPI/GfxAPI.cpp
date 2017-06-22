#include "../PrecompiledHeader.h"
#include "GfxAPI.h"
#include <cassert>
#include "../GfxAPIVulkan/GfxAPIVulkan.h"
#include "../GfxAPINull/GfxAPINull.h"

// Define the instance pointer to null.
GfxAPI * GfxAPI::_apiInstance = nullptr;

// Get the current graphics API instance.
GfxAPI *GfxAPI::Get() {
    assert(_apiInstance != nullptr);
    return _apiInstance; 
}


// Create a Vulkan graphics API.
GfxAPI *GfxAPI::CreateVulkan() {
    assert(_apiInstance == nullptr);
    _apiInstance = new GfxAPIVulkan();
    return _apiInstance;
}


// Create a Null graphics API.
GfxAPI *GfxAPI::CreateNull() {
    assert(_apiInstance == nullptr);
    _apiInstance = new GfxAPINull();
    return _apiInstance;
}
