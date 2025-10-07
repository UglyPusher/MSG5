#include "pch.h"
#include "msg5/config/Sources.h"
#include <utility>

#include "src/providers/ArgsSource.h"
#include "src/providers/FileSource.h"
#include "src/providers/EnvSource.h"
#include "src/providers/PromptSource.h"

namespace msg5::config {

    IOptionsSourcePtr makeArgsSource(int argc, const char* const* argv,
        LogLevel min) noexcept {
        return std::make_unique<ArgsSource>(argc, argv, min);
    }

    IOptionsSourcePtr makeFileSource(std::filesystem::path path_,
        LogLevel min) noexcept {
        return std::make_unique<FileSource>(std::move(path_), min);
    }

    IOptionsSourcePtr makeEnvSource(std::string_view prefix,
        LogLevel min) noexcept {
        return std::make_unique<EnvSource>(std::string(prefix), min);
    }

    IOptionsSourcePtr makePromptSource(bool interactive,
        LogLevel min) noexcept {
        return std::make_unique<PromptSource>(interactive, min);
    }
} // namespace msg5::config
