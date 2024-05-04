#include "pch.hpp"
#include "App.hpp"

#include <filesystem>

int main()
{
#ifdef IDE_ASSET_FOLDER
    std::filesystem::current_path(IDE_ASSET_FOLDER);
#endif

	try {
	    App app{};
        app.Run();
		app.Destroy();
	}
	catch (const std::exception& e) {
        spdlog::error("Unhandled exception: {}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
