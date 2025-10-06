#include "pch.h"
#include "ArgsSource.h"
#include "../util/cli_parse.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {

    ArgsSource::ArgsSource(int argc, const char* const* argv, LogLevel min) noexcept
        : SourceBase(ProviderClass::Cli, "cli")
    {
        set_min_level(min); 
        argv_.reserve(argc > 0 ? argc - 1 : 0);
        //argv_.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            if (i == 0) continue; // пропускаем имя процесса
            argv_.emplace_back(argv[i] ? std::string(argv[i]) : std::string{});
        }
    }

    ArgsSource::ArgsSource(std::vector<std::string> argv, LogLevel min) noexcept
        : SourceBase(ProviderClass::Cli, "cli"), argv_(std::move(argv))
    {
        set_min_level(min);
    }

    void ArgsSource::prepare(const CommandSpec& spec) noexcept {
        flag_to_key_.clear();
        key_flags_.clear();

        flag_to_key_.reserve(flag_to_key_.size() + spec.options.size() * 2);
        key_flags_.reserve(key_flags_.size() + spec.options.size());

        for (const auto& opt : spec.options) {
            // сопоставляем все заявленные cli_flags с каноническим ключом
            for (const auto& fl : opt.cli_flags) {
                if (!fl.empty()) {
                    flag_to_key_.emplace(fl, opt.key);
                    flag_to_key_.emplace(normalize_flag(fl), opt.key);
                }
            }
            key_flags_.emplace(opt.key, opt.flags);
        }
    }

    FetchResult ArgsSource::fetch_impl(const CommandSpec& /*spec*/) noexcept {
        FetchResult out;
        
        if (argv_.empty()) return out;

        auto raw = cli::parse(argv_);

        // Отдаём только ключи, которые есть в спецификации (через кэш flag_to_key_)
        for (const auto& [flag, val] : raw) {
            //auto it = flag_to_key_.find(flag);
            const std::string norm = normalize_flag(flag);
            auto it = flag_to_key_.find(norm);
            if (it == flag_to_key_.end()) {
                // неизвестный флаг — просто сообщим (по ТЗ: игнор/INFO)
                emit(LogLevel::Debug, "CLI_FLAG_UNKNOWN", flag);
                continue;
            }
            const std::string & key = it->second;
            //const bool secret = ((key_flags_.count(key) ? key_flags_.at(key) : 0) & OptSecret) != 0;
            unsigned flags = 0;
            if (auto itf = key_flags_.find(key); itf != key_flags_.end()) flags = itf->second;
            const bool secret = (flags & OptSecret) != 0;
            
            out.kv.emplace(key, val);
            emit(LogLevel::Info,
                "CLI_FLAG_ACCEPTED",
                flag + " -> " + key + (secret ? " = ****" : " = " + val));
        }

        return out;
    }

} // namespace msg5::config
