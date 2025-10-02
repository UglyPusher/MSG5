#include "pch.h"
#include "msg5/config/Sources.h"

// Внутренние реализации провайдеров (прячем их от пользователя либы):
#include "src/providers/ArgsSource.h"
#include "src/providers/FileSource.h"
#include "src/providers/EnvSource.h"
#include "src/providers/PromptSource.h"

namespace msg5::config {

    IOptionsSourcePtr makeArgsSource(int argc, const char* const* argv) {
        return std::make_unique<ArgsSource>(argc, argv);
    }

    IOptionsSourcePtr makeArgsSource(std::vector<std::string> argv) {
        return std::make_unique<ArgsSource>(std::move(argv));
    }

    IOptionsSourcePtr makeFileSource(std::filesystem::path path) {
        return std::make_unique<FileSource>(std::move(path));
    }

    IOptionsSourcePtr makeEnvSource(std::string prefix) {
        return std::make_unique<EnvSource>(std::move(prefix));
    }

    IOptionsSourcePtr makePromptSource(bool interactive) {
        return std::make_unique<PromptSource>(interactive);
    }

} // namespace msg5::config
