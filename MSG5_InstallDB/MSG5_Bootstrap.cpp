#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include <string>
#include <iostream>
#include "bootstrap/commands/Validate.h"
#include "bootstrap/commands/CreateDb.h"
#include "bootstrap/commands/ApplyMetaStructure.h"
#include "bootstrap/commands/ApplyMetaData.h"
#include "bootstrap/UI/UsageText.h"

int main(int argc, char** argv) {
	// Хардкодим простейший разбор команды: argv[1] — имя команды.
	if (argc < 2) {
		msg5::bootstrap::ui::print_usage();
		return 2; // bad usage
	}
	
	const std::string cmd = argv[1];
	
	// Делегируем обработку в BootstrapCli.* (пока пустые тела функций).
	// NB: передаём весь argc/argv — хендлер сам разберёт флаги после имени команды.
	if (cmd == "validate") return msg5::bootstrap::commands::RunValidate(argc, argv);
	else if (cmd == "create-database") return msg5::bootstrap::commands::RunCreateDatabase(argc, argv);
	else if (cmd == "apply-meta-structure") return msg5::bootstrap::commands::RunApplyMetaStructure(argc, argv);
	else if (cmd == "apply-meta-data") return msg5::bootstrap::commands::RunApplyMetaData(argc, argv);
	else {
		std::cerr << "Unknown command: " << cmd << "\n\n";
		msg5::bootstrap::ui::print_usage();
		return 2;
	}
}
