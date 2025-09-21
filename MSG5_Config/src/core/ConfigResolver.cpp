#include "pch.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <algorithm>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

#include "ConfigResolver.h"
#include "CommandSpec.h"
#include "ResolvedOptions.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace msg5::config {

    // ----------------------------- утилиты -----------------------------
    // safe_getenv: кроссплатформенно возвращает значение переменной окружения
    static std::optional<std::string> safe_getenv(const std::string& name) {
#ifdef _WIN32
        char* buf = nullptr;
        size_t len = 0;
        if (_dupenv_s(&buf, &len, name.c_str()) != 0 || !buf) return std::nullopt;
        // _dupenv_s кладёт завершающий '\0' в конец; len включает его
        std::string out = (len > 0) ? std::string(buf, len - 1) : std::string();
        free(buf);
        return out;
#else
        if (const char* v = std::getenv(name.c_str())) return std::string(v);
        return std::nullopt;
#endif
    }

    static bool is_flag(std::string_view s) {
        return s.size() >= 2 && s[0] == '-' && (s[1] == '-' || std::isalpha(static_cast<unsigned char>(s[1])));
    }

    static void set_if_empty(ResolvedOptions& out,
        const std::string& key,
        const std::string& val,
        Origin o) {
        if (!val.empty() && !out.has(key)) {
            out.values[key] = val;
            out.origins[key] = o;
        }
    }

    static std::string json_to_string(const json& v) {
        if (v.is_string())  return v.get<std::string>();
        if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
        if (v.is_number_integer())   return std::to_string(v.get<long long>());
        if (v.is_number_unsigned())  return std::to_string(v.get<unsigned long long>());
        if (v.is_number_float()) { std::ostringstream os; os << v.get<double>(); return os.str(); }
        if (v.is_null()) return {};
        return v.dump(); // массивы/объекты сериализуем целиком (редко, но корректно)
    }

    static std::optional<json> json_at_path(const json& root, const std::string& dotted) {
        if (dotted.empty()) return std::nullopt;
        const json* cur = &root;
        size_t pos = 0;
        while (pos < dotted.size()) {
            size_t dot = dotted.find('.', pos);
            std::string key = dotted.substr(pos, dot == std::string::npos ? dotted.size() - pos : dot - pos);
            if (!cur->is_object() || !cur->contains(key)) return std::nullopt;
            cur = &(*cur)[key];
            if (dot == std::string::npos) break;
            pos = dot + 1;
        }
        return *cur;
    }

    static bool stdin_is_interactive() {
        return isatty(fileno(stdin)) != 0;
    }

    static std::string make_env_key_from_option(const std::string& key) {
        std::string s = key;
        for (auto& c : s) {
            if (c == '.' || c == '-') c = '_';
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s; // e.g. "db.host" -> "DB_HOST"
    }

    // ----------------------------- источники -----------------------------

    // 1) CLI
    static void apply_cli(const CommandSpec& spec, int argc, char** argv, ResolvedOptions& out) {
        // карта флагов → ключ
        std::unordered_map<std::string, const OptionSpec*> flag2opt;
        flag2opt.reserve(spec.options.size() * 2);
        for (const auto& opt : spec.options) {
            for (const auto& f : opt.cli_flags) flag2opt.emplace(f, &opt);
        }

        // Если приложение уже выбрало команду, фиксируем её.
        out.command = spec.name;

        // Поддержим синтаксис: <exe> <command> [flags...] (если вдруг вызывают напрямую)
        int i = 1;
        if (argc > 1 && !is_flag(argv[1])) {
            std::string cmd = argv[1];
            if (!spec.name.empty() && cmd != spec.name) {
                throw std::runtime_error("Unknown command '" + cmd + "' (expected '" + spec.name + "')");
            }
            ++i;
        }

        // Разбор флагов:
        //   --flag=value
        //   --flag value
        //   --bool-flag (для Bool без значения → true)
        for (; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg.rfind("--config=", 0) == 0) {
                out.config_path = fs::path(arg.substr(std::string("--config=").size()));
                continue;
            }
            if (arg == "--config" && (i + 1) < argc) {
                out.config_path = fs::path(argv[++i]);
                continue;
            }

            if (arg.rfind("--", 0) == 0 || (arg.size() >= 2 && arg[0] == '-' && std::isalpha(static_cast<unsigned char>(arg[1])))) {
                std::string flag, val;
                auto eq = arg.find('=');
                if (eq != std::string::npos) {
                    flag = arg.substr(0, eq);
                    val = arg.substr(eq + 1);
                }
                else {
                    flag = arg;
                    if ((i + 1) < argc && !is_flag(argv[i + 1])) {
                        val = argv[++i];
                    }
                }

                auto it = flag2opt.find(flag);
                if (it == flag2opt.end()) {
                    throw std::runtime_error("Unknown CLI flag: " + flag);
                }
                const OptionSpec* os = it->second;

                if (os->type == OptionType::Bool && val.empty()) val = "true";
                if (val.empty() && os->required) {
                    throw std::runtime_error("Option " + flag + " requires a value");
                }

                set_if_empty(out, os->key, val, Origin::Cli);
            }
        }
    }

    // 2) FILE (JSON)
    static void apply_file(const CommandSpec& spec,
        const fs::path& defaultRelConfig,
        ResolvedOptions& out) {
        // Путь к конфигу: --config → defaultRelConfig
        if (out.config_path.empty() && !defaultRelConfig.empty()) {
            out.config_path = defaultRelConfig;
        }

        // config_dir (для относительных путей)
        if (!out.config_path.empty()) {
            fs::path p = out.config_path;
            if (p.is_relative()) p = fs::current_path() / p;
            out.config_path = fs::weakly_canonical(p);
            out.config_dir = out.config_path.parent_path();
        }
        else {
            out.config_dir = fs::current_path();
        }

        // Читаем JSON и маппим по json_path
        if (!out.config_path.empty()) {
            std::ifstream ifs(out.config_path, std::ios::binary);
            if (!ifs) throw std::runtime_error("Cannot open config: " + out.config_path.string());

            json j;
            try { ifs >> j; }
            catch (const std::exception& e) {
                throw std::runtime_error(std::string("Invalid JSON in ") + out.config_path.string() + ": " + e.what());
            }

            for (const auto& opt : spec.options) {
                if (out.has(opt.key) || opt.json_path.empty()) continue;
                auto node = json_at_path(j, opt.json_path);
                if (!node.has_value() || node->is_null()) continue;
                std::string s = json_to_string(*node);
                if (!s.empty()) set_if_empty(out, opt.key, s, Origin::ConfigFile);
            }
        }
    }

    // 3) ENV
    static void apply_env(const CommandSpec& spec,
        const char* envPrefix,
        ResolvedOptions& out) {
        const std::string prefix = (envPrefix && *envPrefix) ? std::string(envPrefix) : std::string();

        for (const auto& opt : spec.options) {
            if (out.has(opt.key)) continue;

            std::vector<std::string> candidates;

            for (const auto& n : opt.env_names) {
                candidates.push_back(n);
                if (!prefix.empty() && n.rfind(prefix, 0) != 0) {
                    candidates.push_back(prefix + "_" + n);
                }
            }

            const std::string derived = make_env_key_from_option(opt.key);
            if (!prefix.empty()) candidates.push_back(prefix + "_" + derived);
            candidates.push_back(derived);

            // dedup
            std::vector<std::string> unique;
            unique.reserve(candidates.size());
            for (auto& n : candidates) {
                if (std::find(unique.begin(), unique.end(), n) == unique.end()) unique.push_back(std::move(n));
            }

            for (const auto& name : unique) {
                if (auto v = safe_getenv(name)) {
                    set_if_empty(out, opt.key, *v, Origin::Env);
                    out.used_env = true;
                    break;
                }
            }
        }
    }


    // 4) STDIN
    static void apply_stdin_if_needed(const CommandSpec& spec,
        ResolvedOptions& out) {
        // Собираем обязательные, которые ещё пустые
        std::vector<const OptionSpec*> missing;
        missing.reserve(spec.options.size());
        for (const auto& opt : spec.options) {
            if (opt.required && !out.has(opt.key)) missing.push_back(&opt);
        }
        if (missing.empty()) return;

        if (!stdin_is_interactive()) {
            std::string msg = "Missing required options (non-interactive stdin): ";
            for (size_t i = 0; i < missing.size(); ++i) {
                if (i) msg += ", ";
                msg += missing[i]->key;
            }
            throw std::runtime_error(msg);
        }

        // Простые промпты через std::getline (без скрытия ввода)
        for (const auto* opt : missing) {
            const std::string label = !opt->prompt.empty() ? opt->prompt : (opt->key + ": ");
            std::string value;

            std::cout << label;
            std::getline(std::cin, value);

            if (value.empty()) {
                throw std::runtime_error("Empty value entered for required option: " + opt->key);
            }
            set_if_empty(out, opt->key, value, Origin::Stdin);
            out.used_stdin = true;
        }
    }

    // ----------------------------- сборка опций -----------------------------

    ResolvedOptions ResolveOptions(const CommandSpec& spec,
        int argc, char** argv,
        const char* envPrefix,
        const fs::path& defaultRelConfig) {
        ResolvedOptions out;

        // 1) CLI
        apply_cli(spec, argc, argv, out);

        // 2) CONFIG FILE
        apply_file(spec, defaultRelConfig, out);

        // 3) ENV
        apply_env(spec, envPrefix, out);

        // 4) STDIN
        apply_stdin_if_needed(spec, out);

        // Дефолты из спецификации
        for (const auto& opt : spec.options) {
            if (!out.has(opt.key) && opt.default_value.has_value()) {
                out.values[opt.key] = *opt.default_value;
                out.origins[opt.key] = Origin::Default;
            }
        }

        // Финальная валидация
        out.validate(spec);

        return out;
    }

    // ----------------------------- validate -----------------------------

    void ResolvedOptions::validate(const CommandSpec& spec) const {
        // Обязательность
        for (const auto& opt : spec.options) {
            if (opt.required && !has(opt.key)) {
                throw std::runtime_error("Missing required option: " + opt.key + " (command: " + spec.name + ")");
            }
        }

        // Enum
        for (const auto& opt : spec.options) {
            if (opt.type == OptionType::Enum && has(opt.key) && !opt.enum_values.empty()) {
                const std::string v = get_str(opt.key);
                bool ok = false;
                for (const auto& ev : opt.enum_values) if (v == ev) { ok = true; break; }
                if (!ok) {
                    std::string msg = "Invalid value '" + v + "' for option " + opt.key + ". Allowed: ";
                    for (size_t i = 0; i < opt.enum_values.size(); ++i) {
                        if (i) msg += ", ";
                        msg += opt.enum_values[i];
                    }
                    throw std::runtime_error(msg);
                }
            }
        }
    }

} // namespace msg5::config
