#include "Application.h"

#include <vector>
#include <stdexcept>
#include <functional>

#include "Options.h"
#include "GfxAPI/GfxAPI.h"


// Run the application - initialize, run the main loop, cleanup at the end.
void Application::Run() {
    // start the graphics API
    InitializeGraphics();
    // program's main loop
    MainLoop();
    // clean up Vulkan API and destroy the application window
    Cleanup();
}


// Start the graphics API and create the window.
void Application::InitializeGraphics() {
    // create the graphics API selected in the options
    const Options &options = Options::Get();
    if (options.GetGfxAPIType() == GfxAPIType::GFX_API_TYPE_VULKAN) {
        apiGfxAPI = GfxAPI::CreateVulkan();
    }
    else if (options.GetGfxAPIType() == GfxAPIType::GFX_API_TYPE_NULL) {
        apiGfxAPI = GfxAPI::CreateNull();
    }
    else {
        throw std::runtime_error("Graphics API type not specified on options");
    }

    // initialize the API and let it create the window
    apiGfxAPI->Initialize(options.GetWindowWidth(), options.GetWindowHeight());
}


// Program's main loop
void Application::MainLoop() {
	// loop until the user closes the window
	while (!GfxAPI::Get()->ShouldCloseWindow()) {
        GfxAPI::Get()->ProcessWindowMessages();
	}
}


// Clean up Vulkan API and destroy the application window
void Application::Cleanup() {
    GfxAPI::Get()->Destroy();
}


