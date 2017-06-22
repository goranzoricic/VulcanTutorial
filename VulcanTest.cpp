#include "PrecompiledHeader.h"
#include <iostream>
#include <stdexcept>

#include "Application.h"


int main() {
	Application app;

	try {
		app.Run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}