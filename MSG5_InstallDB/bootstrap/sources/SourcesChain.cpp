#include "SourcesChain.h"
#include <cstdlib>
#include <string>
#include <string_view>
#include "CliKeys.h"

using namespace msg5::config;

namespace {
#if defined(_MSC_VER)
    static inline std::optional<std::string> GetEnv(std::string_view name) noexcept {
        char* buf = nullptr; size_t len = 0;
        if (_dupenv_s(&buf, &len, std::string(name).c_str()) != 0 || !buf) return std::nullopt;
        std::string v(buf, len ? len - 1 : 0);
        free(buf);
        if (v.empty()) return std::nullopt;
        return v;
    }
#else
    static inline std::optional<std::string> GetEnv(std::string_view name) noexcept {
        if (const char* p = std::getenv(std::string(name).c_str())) {
            if (*p) return std::string(p);
        }
        return std::nullopt;
    }
#endif
} // anon

namespace msg5::bootstrap::sources {

    std::optional<std::filesystem::path>
        FindConfigPath(int argc, const char* const* argv) noexcept {
        for (int i = 1; i + 1 < argc; ++i) if (std::string_view(argv[i]) == "--config")
            return std::filesystem::path(argv[i + 1]);
        if (auto env = GetEnv("MSG5_BOOTSTRAP_CONFIG"); env && !env->empty())
            return std::filesystem::path(*env);
        return std::nullopt;
    }

    std::vector<msg5::config::IOptionsSourcePtr>
        MakeDefaultSources(int argc, const char* const* argv,
            std::optional<std::filesystem::path> cfg) {
        using namespace msg5::config;
        std::vector<IOptionsSourcePtr> s;
        s.push_back(makeArgsSource(argc, argv, LogLevel::Info));         // CLI
        if (cfg) s.push_back(makeFileSource(*cfg, LogLevel::Info));      // CONFIG
        s.push_back(makeEnvSource(K_ENV_PREFIX, LogLevel::Info));        // ENV (MSG5_*)
        s.push_back(makePromptSource(true, LogLevel::Info));             // PROMPT
        return s;
    }

    msg5::config::ResolvedOptions
        ResolveFor(int argc, const char* const* argv,
            const msg5::config::CommandSpec & spec) {
        using namespace msg5::config;
        auto cfg = FindConfigPath(argc, argv);
        Resolver r{ MakeDefaultSources(argc, argv, cfg) };
        r.set_min_level(LogLevel::Info);
        ResolveParams rp;
        rp.apply_defaults = true;
        rp.validate = true;
        rp.log_validation = true;
        return r.resolve(spec, rp);
    }

} // namespace msg5::bootstrap::sources
