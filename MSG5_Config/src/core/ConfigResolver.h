#pragma once
#include <filesystem>
#include "ResolvedOptions.h"
#include "CommandSpec.h"

namespace msg5::config {

    // Собирает опции строго по приоритетам: CLI → FILE → ENV → STDIN
    ResolvedOptions ResolveOptions(const CommandSpec& spec,
        int argc, char** argv,
        const char* envPrefix,
        const std::filesystem::path& defaultRelConfig);

} // namespace msg5::config
