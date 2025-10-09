// MSG5_Config_Tool.cpp — простой CLI для генерации шаблона file-mode
// и дампа «эффективной» конфигурации после резолва.
//
// Пример:
//   MSG5_Config_Tool.exe --db.host=cli.local --db.user=cli ^
//     --gen.template=config/user.template.json ^
//     --dump.effective=config/user.effective.json
//
// Примечание: здесь make_spec() — демонстрационный. В бою подставьте свою фабрику спеки.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include "msg5/config/Resolver.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/Sources.h"
#include "msg5/config/logging/Logger.h"

#include "json.hpp"
using nlohmann::json;
using namespace msg5::config;

#if defined(_WIN32)
#include <windows.h>
#endif

// ---------------------------- Utils: flags -----------------------------------
static std::string get_flag_value(int argc, const char* argv[], std::string_view name) {
    const std::string pref = std::string(name) + "=";
    for (int i = 1; i < argc; ++i) {
        std::string_view a = argv[i];
        if (a.rfind(pref, 0) == 0) return std::string(a.substr(pref.size()));
    }
    return {};
}

static bool has_flag(int argc, const char* argv[], std::string_view name) {
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == name) return true;
    }
    return false;
}

// --------------------------- Example: CommandSpec -----------------------------
// Замените на свою «боевую» спеки (эта — лишь демонстрация).
static CommandSpec make_spec() {
    CommandSpec spec;

    {
        OptionSpec o{};
        o.key = "db.host";
        o.json_path = "db.host";
        o.cli_flags = { "--db.host" };
        o.env_names = {}; // MSG5_DB_HOST тоже ищется EnvSource-ом
        o.flags = 0;
        o.type = OptionType::String;
        o.required = false;
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "db.user";
        o.json_path = "db.user";
        o.cli_flags = { "--db.user" };
        o.env_names = { "DBUSER" };
        o.flags = 0;
        o.type = OptionType::String;
        o.required = false;
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "db.password";
        o.json_path = "db.password";
        o.cli_flags = { "--db.password" };
        o.env_names = { "DBPASS" };
        o.flags = OptSecret;
        o.type = OptionType::String;
        o.required = false;
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "log.level";
        o.json_path = "log.level";
        o.cli_flags = { "--log.level" };
        o.flags = 0;
        o.type = OptionType::Enum;
        o.enum_values = { "trace","debug","info","warn","error" };
        o.default_value = std::string("info"); // дефолт
        o.required = false;
        spec.options.push_back(std::move(o));
    }

    return spec;
}

// --------------------------- IO helpers (tool-only) ---------------------------
static std::unordered_map<std::string, unsigned> build_flag_map(const CommandSpec& spec) {
    std::unordered_map<std::string, unsigned> m;
    m.reserve(spec.options.size());
    for (const auto& o : spec.options) m[o.key] = o.flags;
    return m;
}

static std::string mask_secret(const std::string& s) {
    if (s.empty()) return s;
    if (s.size() <= 4) return "****";
    return s.substr(0, 2) + std::string(s.size() - 2, '*');
}

static bool write_template_json(const CommandSpec& spec, const std::filesystem::path& path) {
    json j = json::object();
    for (const auto& o : spec.options) {
        if (o.default_value.has_value()) j[o.key] = *o.default_value;
        else if (o.required) {
            j[o.key] = "";
            j[o.key + ".__comment"] = "required; fill me";
        }
        else {
            j[o.key] = "";
        }
        if (o.type == OptionType::Enum && !o.enum_values.empty()) {
            j[o.key + ".__enum"] = o.enum_values;
        }
    }
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << j.dump(2);
    return true;
}

static bool write_effective_json(const CommandSpec& spec, const ResolvedOptions& ro,
    const std::filesystem::path& path,
    bool mask_secrets = true,
    bool include_source = true) {
    const auto flags_by_key = build_flag_map(spec);
    json j = json::object();
    json src = json::object();

    for (const auto& [k, v] : ro.values) {
        const bool is_secret = ((flags_by_key.find(k) != flags_by_key.end()) &&
            (flags_by_key.at(k) & OptSecret)) != 0;
        j[k] = (mask_secrets && is_secret) ? mask_secret(v) : v;
        if (include_source) {
            auto it = ro.value_sources.find(k);
            src[k] = (it == ro.value_sources.end()) ? to_string(ValueSource::Default)
                : to_string(it->second);
        }
    }
    if (include_source) j["__source"] = src;

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << j.dump(2);
    return true;
}

// --------------------------- Sources builder ----------------------------------
static std::vector<IOptionsSourcePtr> make_sources(int argc, const char* argv[], bool prompt_interactive) {
    std::vector<IOptionsSourcePtr> sources;
    sources.emplace_back(makeArgsSource(argc, argv));
    sources.emplace_back(makeFileSource("config/user.json"));
    sources.emplace_back(makeEnvSource("MSG5_"));
    sources.emplace_back(makePromptSource(prompt_interactive));
    return sources;
}

// --------------------------- Logging bridge -----------------------------------
static void print_log(const LogEvent& ev) {
    const char* lvl =
        ev.level == LogLevel::Error ? "ERR" :
        ev.level == LogLevel::Warn ? "WARN" :
        ev.level == LogLevel::Info ? "INFO" :
        ev.level == LogLevel::Debug ? "DBG" : "TRC";
    std::cout << "[log] " << lvl
        << " src=" << ev.source_id
        << " code=" << ev.code
        << " msg=" << ev.message << "\n";
}

static void print_help() {
    std::cout <<
        R"(MSG5_Config_Tool — генератор шаблонов и дампер эффективной конфигурации

Использование:
MSG5_Config_Tool.exe [опции-конфига] [опции-тула]

Опции конфига (зависят от вашей CommandSpec):
  --db.host=HOST            Хост БД
  --db.user=USER            Пользователь БД
  --db.password=PASS        Пароль БД (секрет; в дампе маскируется)
  --log.level=LEVEL         trace|debug|info|warn|error

Опции тула:
  --gen.template=PATH       Сгенерировать файл-шаблон (плоский JSON c dotted-ключами)
  --dump.effective=PATH     Сохранить «эффективную» конфигурацию после resolve
  --no-prompt               Отключить интерактивные вопросы (PromptSource)
  --help | -h | /?          Показать эту справку и выйти

Примеры:
  MSG5_Config_Tool.exe --gen.template=config/user.template.json
  MSG5_Config_Tool.exe --db.host=cli --db.user=cli --no-prompt ^
                       --dump.effective=config/user.effective.json

Заметки:
  • Формат файла — плоский JSON (dotted keys), совместим с текущим FileSource.
  • Секреты (OptSecret) в дампе маскируются по умолчанию.
  • Порядок источников: CLI > FILE(config/user.json) > ENV(MSG5_*) > PROMPT.
)";
}

// Отфильтровать флаги тула из argv, чтобы ArgsSource их не видел и не логировал как unknown.
static void build_filtered_argv(int argc, const char* argv[],
    std::vector<std::string>&out_args,
    std::vector<const char*>&out_c)
    {
    auto is_tool_flag = [](std::string_view a) {
            return a == "--no-prompt"
            || a == "--help" || a == "-h" || a == "/?" 
            || a.rfind("--gen.template=", 0) == 0
            || a.rfind("--dump.effective=", 0) == 0;
        };
    out_args.clear();
    out_args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        std::string_view a = argv[i];
        if (i == 0 || !is_tool_flag(a)) { // argv[0] оставляем всегда
            out_args.emplace_back(a);
        }
    }
    
    out_c.clear();
    out_c.reserve(out_args.size());
    for (auto& s : out_args) out_c.push_back(s.c_str());
}

// ------------------------------ main ------------------------------------------
int main(int argc, const char* argv[]) {
    try {
#if defined(_WIN32)
        // Переключаем ввод/вывод консоли на UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
        // Справка — до любых действий
        if (has_flag(argc, argv, "--help") || has_flag(argc, argv, "-h") || has_flag(argc, argv, "/?")) {
            print_help();
            return 0;
        }

        std::cout << "[cwd] " << std::filesystem::current_path().string() << "\n";
        const auto p = std::filesystem::path("config/user.json");
        std::cout << "[file] " << p.string() << " exists=" << std::filesystem::exists(p) << "\n";

        // Режимы утилиты
        const std::string tmpl_path = get_flag_value(argc, argv, "--gen.template");
        const std::string dump_path = get_flag_value(argc, argv, "--dump.effective");
        const bool prompt_interactive = !has_flag(argc, argv, "--no-prompt");

        // Спека (в реальном приложении — отдаёт ваш модуль конфигурации)
        CommandSpec spec = make_spec();

        // 1) Только генерация шаблона — можно делать без резолва
        if (!tmpl_path.empty()) {
            if (write_template_json(spec, tmpl_path)) {
                std::cout << "[io] template written: " << tmpl_path << "\n";
            }
            else {
                std::cerr << "[io] template write FAILED: " << tmpl_path << "\n";
                return 2;
            }
        }

        // 2) Резолв (+ дефолты + валидация)
        //    Сначала фильтруем argv от флагов тула (чтобы ArgsSource не видел их).
        std::vector<std::string> filtered;
        std::vector<const char*> av;
        build_filtered_argv(argc, argv, filtered, av);
        
        Resolver resolver(make_sources(static_cast<int>(av.size()), av.data(), prompt_interactive));
        resolver.set_min_level(LogLevel::Trace);
        resolver.subscribe([](const LogEvent& ev) { print_log(ev); });

        ResolveParams params{};
        params.apply_defaults = true;
        params.validate = true;
        params.log_validation = true;

        ResolvedOptions::ValidationResult vr;
        params.out_errors = &vr;

        ResolvedOptions ro = resolver.resolve(spec, params);

        if (!vr.empty()) {
            std::cout << "=== Validation errors ===\n";
            for (const auto& e : vr) {
                std::cout << " - " << e.key << ": " << e.message << "\n";
            }
            // политика утилиты: вернуть код 3 при ошибке валидации
            // (если не нужно — меняй на 0)
        }

        // 3) Дамп эффективной конфигурации (опционально)
        if (!dump_path.empty()) {
            if (write_effective_json(spec, ro, dump_path, /*mask_secrets=*/true, /*include_source=*/true)) {
                std::cout << "[io] effective config dumped: " << dump_path << "\n";
            }
            else {
                std::cerr << "[io] effective dump FAILED: " << dump_path << "\n";
                return 4;
            }
        }

        return vr.empty() ? 0 : 3;
    }
    catch (const std::exception& ex) {
        std::cerr << "[fatal] " << ex.what() << "\n";
        return 1;
    }
}
