#include "pch.h"

#include <algorithm> // std::replace
#include "ArgsSource.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {
    // ArgsSource.cpp
    static bool is_flag_like_sv(std::string_view s) noexcept {
        return !s.empty() && (s[0] == '-' || s[0] == '/');
    }

    std::vector<std::pair<std::string, std::string>>
        ArgsSource::parse_argv_pairs(const std::vector<std::string>& argv) {
        std::vector<std::pair<std::string, std::string>> out;
        if (argv.empty()) return out;

        // argv[0] — имя процесса; начинаем с 1
        for (size_t i = 1; i < argv.size(); ++i) {
            const std::string& a = argv[i];

            // Длинные: --key[=value]
            if (a.rfind("--", 0) == 0) {
                std::string body = a.substr(2);
                const auto eq = body.find('=');
                if (eq != std::string::npos) {
                    std::string key = body.substr(0, eq);
                    std::string val = body.substr(eq + 1);
                    if (!key.empty()) out.emplace_back(std::move(key), std::move(val));
                    continue;
                }
                // Вид: --key value  или просто --key (true)
                std::string key = std::move(body);
                std::string val = "true";
                if (i + 1 < argv.size() && !is_flag_like_sv(argv[i + 1])) {
                    val = argv[++i];
                }
                if (!key.empty()) out.emplace_back(std::move(key), std::move(val));
                continue;
            }

            // Короткие пачкой: -abc  → a=true,b=true,c=true
            if (a.size() > 1 && a[0] == '-' && a[1] != '-') {
                for (size_t j = 1; j < a.size(); ++j) {
                    char c = a[j];
                    if (!std::isspace(static_cast<unsigned char>(c))) {
                        std::string key(1, c);
                        out.emplace_back(std::move(key), std::string("true"));
                    }
                }
                continue;
            }

            // Позиционные игнорируем (по текущему ТЗ)
        }
        return out;
    }


    ArgsSource::ArgsSource(int argc, const char* const* argv, LogLevel min) noexcept
        : SourceBase(ProviderClass::Cli, "cli")
    {
        set_min_level(min); 
        argv_.reserve(argc > 0 ? argc - 1 : 0);
        //argv_.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            //if (i == 0) continue; // пропускаем имя процесса
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

        flag_to_key_.reserve(flag_to_key_.size() + spec.options.size() * 8);
        key_flags_.reserve(key_flags_.size() + spec.options.size());

        for (const auto& opt : spec.options) {
            // сопоставляем все заявленные cli_flags с каноническим ключом
            for (const auto& fl : opt.cli_flags) {
                if (!fl.empty()) {
                    flag_to_key_.emplace(fl, opt.key);
                    flag_to_key_.emplace(normalize_flag(fl), opt.key);
                    // TRACE: что зарегистрировали
                    emit(LogLevel::Trace, "CLI_FLAG_REGISTER",
                        std::string("key=") + opt.key +
                        " flag=" + fl +
                        " norm=" + normalize_flag(fl));
                    // Неявные алиасы: db.host ⇄ db-host ⇄ db_host
                    const std::string base = normalize_flag(fl); // напр., "db.host"
                    if (base.find('.') != std::string::npos) {
                        std::string dash = base;  std::replace(dash.begin(), dash.end(), '.', '-'); // db-host
                        std::string und = base;  std::replace(und.begin(), und.end(), '.', '_'); // db_host
                        flag_to_key_.emplace(dash, opt.key);
                        flag_to_key_.emplace(und, opt.key);
                        emit(LogLevel::Trace, "CLI_ALIAS_REGISTER",
                            std::string("key=") + opt.key +
                            " alias=" + dash + "," + und);
                    }
                }
            }
            key_flags_.emplace(opt.key, opt.flags);
        }
    }

    FetchResult ArgsSource::fetch_impl(const CommandSpec& /*spec*/) noexcept {
        FetchResult out;
        
        if (argv_.empty()) return out;

        auto raw = parse_argv_pairs(argv_);

        // Отдаём только ключи, которые есть в спецификации (через кэш flag_to_key_)
        for (const auto& [flag_raw, val_raw] : raw) {
            // Логируем, что реально пришло от парсера
            emit(LogLevel::Trace, "CLI_TOKEN_RAW",
                std::string("flag=") + flag_raw + " val=" + val_raw);
            std::string flag = flag_raw;
            std::string value = val_raw;
            
            // Фоллбэк: если парсер не выделил значение, но форма "--k=v" — разберём сами
            if (value.empty()) {
                const auto eq = flag.find('=');
                if (eq != std::string::npos) {
                    value = flag.substr(eq + 1);
                    flag = flag.substr(0, eq);
                    emit(LogLevel::Trace, "CLI_TOKEN_EQ_SPLIT",
                        std::string("flag=") + flag + " val=" + value);
                }
            }
            
            const std::string norm = normalize_flag(flag);
            auto it = flag_to_key_.find(norm);
            if (it == flag_to_key_.end()) {
                // неизвестный флаг — покажем и сырой, и нормализованный
                emit(LogLevel::Debug, "CLI_FLAG_UNKNOWN",
                    std::string("raw=") + flag_raw + " norm=" + norm);
                continue;
            }
            const std::string & key = it->second;
            //const bool secret = ((key_flags_.count(key) ? key_flags_.at(key) : 0) & OptSecret) != 0;
            unsigned flags = 0;
            if (auto itf = key_flags_.find(key); itf != key_flags_.end()) flags = itf->second;
            const bool secret = (flags & OptSecret) != 0;
            
            out.kv.emplace(key, value);
            emit(LogLevel::Info,
                "CLI_FLAG_ACCEPTED",
                flag + " -> " + key + (secret ? " = ****" : " = " + value));
        }

        return out;
    }

} // namespace msg5::config
