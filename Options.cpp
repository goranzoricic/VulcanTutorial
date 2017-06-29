#include "PrecompiledHeader.h"
#include "Options.h"


// Default constructor initializes the options to hardcoded values (i.e. defaults).
Options::Options()
{
    // default window size is 1080p
    _dimWindowWidth = 1920;
    _dimWindowHeight = 1080;

    // use the Vulkan APi by default
    _optGfxAPIType = GfxAPIType::GFX_API_TYPE_VULKAN;

    // Vulkan specific

    // enable validation layers only in debug builds
    #ifdef NDEBUG
        _optShouldUseValiationLayers = false;
    #else
        _optShouldUseValiationLayers = true;
    #endif
}


