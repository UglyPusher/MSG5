#include "pch.h"
#include "msg5/config/Sources.h"

// Внутренние реализации провайдеров (прячем их от пользователя либы):
#include "src/providers/ArgsSource.h"
#include "src/providers/FileSource.h"
#include "src/providers/EnvSource.h"
#include "src/providers/PromptSource.h"

namespace msg5::config {

    IOptionsSourcePtr makeArgsSource(int argc, const char* const* argv
        , LogLevel min) { return std::make_unique<ArgsSource>(argc, argv, min); }

    IOptionsSourcePtr makeArgsSource(std::vector<std::string> argv) {
        return std::make_unique<ArgsSource>(std::move(argv));
    }

    IOptionsSourcePtr makeFileSource(const std::filesystem::path& p, LogLevel min) {
        return std::make_unique<FileSource>(p, min);
    }

    IOptionsSourcePtr makeEnvSource(std::string prefix, LogLevel min) { 
        return std::make_unique<EnvSource>(std::string(prefix), min); 
    }

    IOptionsSourcePtr makePromptSource(bool interactive, LogLevel min) {
        return std::make_unique<PromptSource>(interactive, min);
    }

} // namespace msg5::config
