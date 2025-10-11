#pragma once
#include <optional>
#include <filesystem>
#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/Sources.h"
#include "msg5/config/Resolver.h"

namespace msg5::bootstrap::sources {
    [[nodiscard]]  std::optional<std::filesystem::path> FindConfigPath(int argc, const char* const* argv) noexcept;
    std::vector<msg5::config::IOptionsSourcePtr>
        MakeDefaultSources(int argc, const char* const* argv,
            std::optional<std::filesystem::path> cfg);

    msg5::config::ResolvedOptions ResolveFor(int argc, const char* const* argv,
        const msg5::config::CommandSpec& spec);
} // namespace msg5::bootstrap::sources
