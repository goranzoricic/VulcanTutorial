#pragma once
#include "GfxAPI/GfxAPI.h"

// Implementation of application options. They are implemented as a singleton with read-only access from the outside.
// This is the initial implementation witht he intention to have all options in one place and not scattered around the code.
// Future implementation should add loading/saving options from a file, changing them on the fly, marking as 'dirty' when changed etc.
class Options
{
public:
    // Singleton getter for the options.
    static const Options &Get() {
        static Options singOptions;
        return singOptions;
    }

public:
    // Get the desired width of the application window.
    uint32_t GetWindowWidth() const { return _dimWindowWidth; }
    // Get the desired height of the application window.
    uint32_t GetWindowHeight() const { return _dimWindowHeight; }

    // Get the graphics API type the application should use.
    enum GfxAPIType GetGfxAPIType() const { return _optGfxAPIType; }

    // Vulkan specific

    // Should the application use validation layers and error callback?
    bool ShouldUseValidationLayers() const { return _optShouldUseValiationLayers;  }

private:
    // Options objects shouldnt be created or destroyed from the outside.
    Options();
    ~Options() {};

private:
    // Width and height of the application window.
    uint32_t _dimWindowWidth;
    uint32_t _dimWindowHeight;

    // Which graphics API should the application use (Vulkan/Null...)
    enum GfxAPIType _optGfxAPIType;

    // Vulkan specific

    // Should the application use validation layers and error callback?
    bool _optShouldUseValiationLayers;
};

