#include "GPUInstancing.hpp"

#include "helpers/pch.hpp"
#include "helpers/App.hpp"

#include <filesystem>

int main()
{
#ifdef IDE_ASSET_FOLDER
    std::filesystem::current_path(IDE_ASSET_FOLDER);
#endif

	App app{};

	try {
		app.Start();
	}
	catch (const std::exception& e) {
		std::cerr << "!ERROR!\n";
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
