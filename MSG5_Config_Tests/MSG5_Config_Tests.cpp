// MSG5_Config_Tests.cpp — примеры использования Resolver с дефолтами и валидацией.
// Сценарии:
//  1) Позитив: подставляем default_value и валидируем — всё ок.
//  2) Негатив: намеренно "ломаем" enum (log.level=verbose) — ловим VALIDATION_ERROR.
//
// Сборка/запуск (пример):
//   x64\Release\MSG5_Config_Tests.exe --db.host=cli.local --db.user=cli
//   x64\Release\MSG5_Config_Tests.exe --db.host=cli.local --db.user=cli --log.level=verbose  (ожидаем ошибку)

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <filesystem>

#include "msg5/config/Resolver.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/Sources.h"              // makeArgsSource/makeFileSource/makeEnvSource/makePromptSource
#include "msg5/config/logging/Logger.h"       // LogEvent, LogLevel, to_string(ValueSource)

using namespace msg5::config;

// ---------------------------------------------------------------
// Утилита печати лог-событий (единый формат для всех источников и резолвера)
static inline void print_log(const LogEvent& ev) {
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

// Печать эффективной конфигурации
static void print_effective(const ResolvedOptions& ro) {
    std::cout << "=== Effective config ===\n";
    for (const auto& [k, v] : ro.values) {
        ValueSource src = ValueSource::Default;
        if (auto it = ro.value_sources.find(k); it != ro.value_sources.end()) {
            src = it->second;
        }
        std::cout << "  " << k << " = " << v << "  [" << to_string(src) << "]\n";
    }
}

// ---------------------------------------------------------------
// Пример спецификации без designator-инициализаторов.
//  - db.host/db.user/db.password — строковые поля
//  - log.level — Enum с допустимыми значениями + default_value = "info"
static CommandSpec make_spec() {
    CommandSpec spec;

    {
        OptionSpec o{};
        o.key = "db.host";
        o.json_path = "db.host";
        o.cli_flags = { "--db.host" };
        o.env_names = {};               // также ищется MSG5_DB_HOST
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
        o.flags = OptSecret;            // маскирование в логах
        o.type = OptionType::String;
        o.required = false;
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "log.level";
        o.json_path = "log.level";
        o.cli_flags = { "--log.level" };
        o.env_names = {};               // MSG5_LOG_LEVEL
        o.flags = 0;
        o.type = OptionType::Enum;
        o.enum_values = { "trace","debug","info","warn","error" };
        o.default_value = std::string("info"); // дефолт, если поле отсутствует
        o.required = false;
        spec.options.push_back(std::move(o));
    }

    return spec;
}

// ---------------------------------------------------------------
// Сбор источников. Порядок = приоритет (ранний сильнее позднего):
// CLI > FILE > ENV > PROMPT.
static std::vector<IOptionsSourcePtr> make_sources(int argc, const char* argv[], bool prompt_interactive) {
    std::vector<IOptionsSourcePtr> sources;
    sources.emplace_back(makeArgsSource(argc, argv));
    sources.emplace_back(makeFileSource("config/user.json"));
    sources.emplace_back(makeEnvSource("MSG5_"));
    sources.emplace_back(makePromptSource(prompt_interactive));
    return sources;
}

// Единая обвязка: resolve → (defaults) → (validate), с выводом логов и ошибок.
static ResolvedOptions resolve_with_policy(
    Resolver& r,
    const CommandSpec& spec,
    bool apply_defaults,
    bool do_validate,
    ResolvedOptions::ValidationResult* out_errors // может быть nullptr
) {
    ResolveParams p{};
    p.apply_defaults = apply_defaults;
    p.validate = do_validate;
    p.log_validation = true;
    p.out_errors = out_errors;

    return r.resolve(spec, p);
}

// ---------------------------------------------------------------
// Позитивный сценарий: дефолты применяются, валидация проходит.
// Ожидаем, что при отсутствии log.level в источниках дефолт "info" подставится.
static void scenario_positive(int argc, const char* argv[]) {
    CommandSpec spec = make_spec();

    // лёгкий переключатель для PromptSource
    bool prompt_interactive = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--no-prompt") {
            prompt_interactive = false;
        }
    }

    Resolver r(make_sources(argc, argv, prompt_interactive));
    r.set_min_level(LogLevel::Trace);
    r.subscribe([](const LogEvent& ev) { print_log(ev); });

    ResolvedOptions::ValidationResult vr;
    ResolvedOptions ro = resolve_with_policy(r, spec, /*defaults*/true, /*validate*/true, &vr);

    // Ожидаем отсутствие ошибок валидации
    assert(vr.empty() && "Validation must pass in positive scenario");

    // Если log.level не был передан через CLI/FILE/ENV — должен быть Default=info
    const std::string lvl = ro.get_str("log.level", "");
    assert(!lvl.empty());
    // Источник либо Default, либо то, что пришло из конфигов/CLI.
    // Проверим, что ключ вообще присутствует:
    assert(ro.has("log.level"));

    print_effective(ro);
}

// Негативный сценарий: специально задаём недопустимое значение enum (например, verbose)
// и проверяем, что валидатор находит ошибку.
static void scenario_negative_enum(int argc, const char* argv[]) {
    // Сконструируем фальшивые argv, добавив "--log.level=verbose" к реальным аргументам.
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc) + 1);
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
    args.emplace_back("--log.level=verbose");

    // Превратим обратно в массив C-строк
    std::vector<const char*> av;
    av.reserve(args.size());
    for (const auto& s : args) av.push_back(s.c_str());

    CommandSpec spec = make_spec();
    bool prompt_interactive = false; // не нужен интерактив в негативном тесте

    Resolver r(make_sources(static_cast<int>(av.size()), av.data(), prompt_interactive));
    r.set_min_level(LogLevel::Trace);
    r.subscribe([](const LogEvent& ev) { print_log(ev); });

    ResolvedOptions::ValidationResult vr;
    ResolvedOptions ro = resolve_with_policy(r, spec, /*defaults*/true, /*validate*/true, &vr);

    // Ожидаем, что найдётся хотя бы одна ошибка и среди них — по ключу log.level
    assert(!vr.empty() && "Validation must fail for invalid enum");
    bool has_log_level_error = false;
    for (const auto& e : vr) {
        if (e.key == "log.level") { has_log_level_error = true; break; }
    }
    assert(has_log_level_error && "Validation must report error for log.level");

    // Печать результата и ошибок
    print_effective(ro);
    std::cout << "\n=== Validation errors ===\n";
    for (const auto& e : vr) {
        std::cout << " - " << e.key << ": " << e.message << "\n";
    }
}

// ---------------------------------------------------------------
int main(int argc, const char* argv[]) {
    // Инфо-блок
    std::cout << "[cwd] " << std::filesystem::current_path().string() << "\n";
    const auto p = std::filesystem::path("config/user.json");
    std::cout << "[file] " << p.string() << " exists=" << std::filesystem::exists(p) << "\n";

    // 1) Позитивный пример использования
    scenario_positive(argc, argv);

    // 2) Негативный пример (ломаем enum)
    scenario_negative_enum(argc, argv);

    std::cout << "\nMSG5_Config_Tests done.\n";
    return 0;
}
