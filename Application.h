#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Application {
public:
    Application() : apiGfxAPI(nullptr) {}

    // Run the application - initialize, run the main loop, cleanup at the end.
	void Run();

private:
    // Grapics API to use in the application.
    class GfxAPI *apiGfxAPI;

    // Start the graphics API and create the window.
    void InitializeGraphics();
	// Program's main loop
	void MainLoop();
	// Clean up Vulkan API and destroy the application window
	void Cleanup();
};
