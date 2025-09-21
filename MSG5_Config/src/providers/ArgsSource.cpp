#include "pch.h"
#include "ArgsSource.h"
#include "../util/cli_parse.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {

    ArgsSource::ArgsSource(int argc, const char* const* argv)
        : SourceBase(SourceKind::Cli) {
        argv_.reserve(static_cast<size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            argv_.emplace_back(argv[i] ? argv[i] : "");
        }
    }

    ArgsSource::ArgsSource(std::vector<std::string> argv)
        : SourceBase(SourceKind::Cli), argv_(std::move(argv)) {
    }

    FetchResult ArgsSource::fetch(const CommandSpec& spec) {
        FetchResult r;
        if (argv_.empty()) return r;

        auto raw = cli::parse(argv_);

        // Пока берём всё как есть (фильтрацию по spec включим позже)
        for (auto& kvp : raw) {
            r.kv[kvp.first] = kvp.second;
        }

        return r;
    }

} // namespace msg5::config
