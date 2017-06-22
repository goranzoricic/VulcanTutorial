#pragma once
#include "../GfxAPI/GfxAPI.h"

// Implementation of Vulkan graphics API.
class GfxAPIVulkan : public GfxAPI {
private:
    GfxAPIVulkan() {};
    ~GfxAPIVulkan() {};
    friend class GfxAPI;
};

