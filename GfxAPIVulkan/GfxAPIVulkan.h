#pragma once
#include "../GfxAPI/GfxAPI.h"

// Implementation of Vulkan graphics API.
class GfxAPIVulkan : public GfxAPI {
private:
    GfxAPIVulkan() {};
    ~GfxAPIVulkan() {};
    friend class GfxAPI;

public:
    // Initialize the API. Returns true if successfull.
    virtual bool Initialize(uint32_t dimWidth, uint32_t dimHeight);
    // Destroy the API. Returns true if successfull.
    virtual bool Destroy();
};

