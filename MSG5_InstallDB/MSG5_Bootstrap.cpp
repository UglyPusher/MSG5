#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include <string>
#include <iostream>
#include "BootstrapCli.h"
#include "bootstrap/UsageText.h"

int main(int argc, char** argv) {
	// Хардкодим простейший разбор команды: argv[1] — имя команды.
	if (argc < 2) {
		bootstrap::print_usage();
		return 2; // bad usage
	}
	
	const std::string cmd = argv[1];
	
	// Делегируем обработку в BootstrapCli.* (пока пустые тела функций).
	// NB: передаём весь argc/argv — хендлер сам разберёт флаги после имени команды.
	if (cmd == "validate") {
		return HandleValidate(argc, argv);
	}
	else if (cmd == "create-database") {
		return HandleCreateDatabase(argc, argv);
	}
	else if (cmd == "apply-meta-structure") {
		return HandleApplyMetaStructure(argc, argv);
	}
	else if (cmd == "apply-meta-data") {
		return HandleApplyMetaData(argc, argv);
	}
	else {
		std::cerr << "Unknown command: " << cmd << "\n\n";
		bootstrap::print_usage();
		return 2;
	}
}
