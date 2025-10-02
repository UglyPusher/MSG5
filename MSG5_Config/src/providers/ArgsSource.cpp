#include "pch.h"
#include "ArgsSource.h"
#include "../util/cli_parse.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {

    ArgsSource::ArgsSource(int argc, const char* const* argv)
        : SourceBase(ProviderClass::Cli, "cli") {
        argv_.reserve(static_cast<size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            argv_.emplace_back(argv[i] ? argv[i] : "");
        }
    }

    ArgsSource::ArgsSource(std::vector<std::string> argv)
        : SourceBase(ProviderClass::Cli, "cli"), argv_(std::move(argv)) {
    }

    void ArgsSource::prepare(const CommandSpec& spec) {
        flag_to_key_.clear();
        key_flags_.clear();
        for (const auto& opt : spec.options) {
            // сопоставляем все заявленные cli_flags с каноническим ключом
            for (const auto& fl : opt.cli_flags) {
                if (!fl.empty()) flag_to_key_.emplace(fl, opt.key);
            }
            key_flags_.emplace(opt.key, opt.flags);
        }
    }

    FetchResult ArgsSource::fetch_impl(const CommandSpec& /*spec*/) {
        FetchResult out;
        
        if (argv_.empty()) return out;

        auto raw = cli::parse(argv_);

        // Отдаём только ключи, которые есть в спецификации (через кэш flag_to_key_)
        for (const auto& [flag, val] : raw) {
            auto it = flag_to_key_.find(flag);
            if (it == flag_to_key_.end()) {
                // неизвестный флаг — просто сообщим (по ТЗ: игнор/INFO)
                emit(LogLevel::Info, "CLI_FLAG_UNKNOWN", flag);
                continue;
            }
            const std::string & key = it->second;
            const bool secret = ((key_flags_.count(key) ? key_flags_.at(key) : 0) & OptSecret) != 0;
            out.kv.emplace(key, val);
            emit(LogLevel::Info,
                "CLI_FLAG_ACCEPTED",
                flag + " -> " + key + (secret ? " = ****" : " = " + val));
        }

        return out;
    }

} // namespace msg5::config
