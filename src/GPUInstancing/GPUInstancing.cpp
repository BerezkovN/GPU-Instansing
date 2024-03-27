#include "GPUInstancing.hpp"

#include "helpers/pch.hpp"
#include "helpers/App.hpp"

int main()
{
	App app{};

	try {
		app.Start();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
