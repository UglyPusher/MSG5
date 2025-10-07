#include <iostream>
#include <string>
#include <vector>
#include <cassert>

#include "msg5/config/Resolver.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h"      // ResolvedOptions (+ тянет ValueSource)
#include "msg5/config/Sources.h"              // makeArgsSource/makeFileSource/makeEnvSource/makePromptSource

#include <filesystem>
#include "msg5/config/logging/Logger.h"      // LogEvent, LogLevel

using namespace msg5::config;

// Подписка на события всех источников: печать в консоль
static inline void print_log(const LogEvent & ev) {
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


// Мини-утилита печати результата
static void print_effective(const ResolvedOptions& ro) {
    std::cout << "=== Effective config ===\n";
    for (const auto& kv : ro.values) {
        const std::string& k = kv.first;
        const std::string& v = kv.second;

        ValueSource src = ValueSource::Default;
        if (auto it = ro.value_sources.find(k); it != ro.value_sources.end()) {
            src = it->second;
        }
        std::cout << "  " << k << " = " << v << "  [" << to_string(src) << "]\n";
    }
}

// Спецификация опций без designator-инициализаторов
static CommandSpec make_spec() {
    CommandSpec spec;

    {
        OptionSpec o{};
        o.key = "db.host";
        o.json_path = "db.host";              // для плоского JSON — совпадает с key
        o.cli_flags = { "--db.host" };
        o.env_names = {};                     // будет искать MSG5_DB_HOST
        o.flags = 0;
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "db.user";
        o.json_path = "db.user";
        o.cli_flags = { "--db.user" };
        o.env_names = { "DBUSER" };             // явный ENV-алиас
        o.flags = 0;
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "db.password";
        o.json_path = "db.password";
        o.cli_flags = { "--db.password" };
        o.env_names = { "DBPASS" };             // явный ENV-алиас
        o.flags = OptSecret;              // значение будет маскировано в логах
        spec.options.push_back(std::move(o));
    }
    {
        OptionSpec o{};
        o.key = "log.level";
        o.json_path = "log.level";
        o.cli_flags = { "--log.level" };
        o.env_names = {};                     // MSG5_LOG_LEVEL
        o.flags = 0;
        spec.options.push_back(std::move(o));
    }

    return spec;
}

// Базовый смок «как есть»: Args > File > Env > Stdin
static ResolvedOptions run_basic(int argc, const char* argv[]) {
    CommandSpec spec = make_spec();

    // лёгкий разбор флага --prompt (включает интерактивный режим PromptSource)
    bool prompt_interactive = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--no-prompt") {
            prompt_interactive = false;
        }
        else if ((std::string_view(argv[i]) == "--app-log-level")) {}
    }

    std::vector<IOptionsSourcePtr> sources;
    sources.emplace_back(makeArgsSource(argc, argv));
    sources.emplace_back(makeFileSource("config/user.json")); // можно убрать/переименовать для проверки FILE_NOT_FOUND
    sources.emplace_back(makeEnvSource("MSG5_"));
    sources.emplace_back(makePromptSource(prompt_interactive));
    
    Resolver r(std::move(sources));
    r.set_min_level(LogLevel::Trace); // видеть всё, как в вашем выводе
    r.subscribe([](const LogEvent& ev) { print_log(ev); });
    return r.resolve(spec);
}

// Небольшие sanity-проверки порядка слоёв (без тяжёлого фреймворка)
static void simple_asserts(const ResolvedOptions& ro, bool expect_file_for_log_level) {
    // Если в файле присутствовал log.level — ожидаем ValueSource::ConfigFile
    if (expect_file_for_log_level) {
        auto it = ro.value_sources.find("log.level");
        assert(it != ro.value_sources.end() && "log.level must exist");
        assert(it->second == ValueSource::ConfigFile && "log.level source must be ConfigFile");
    }

    // Если CLI передан как --db.host=..., то он должен быть сильнее (ранний)
    (void)ro; // чтобы не ругался компилятор, если не используем
}

int main(int argc, const char* argv[]) {

    std::cout << "[cwd] " << std::filesystem::current_path().string() << "\n";
    const auto p = std::filesystem::path("config/user.json");
    std::cout << "[file] " << p.string() << " exists=" << std::filesystem::exists(p) << "\n";

    // Сценарий: смотрим, что собралось, и печатаем
    const ResolvedOptions ro = run_basic(argc, argv);

    // Считаем, что в репозитории присутствует пример config/user.json с полем log.level,
    // тогда ожидаем ValueSource::ConfigFile для log.level. При необходимости выставьте флаг в false.
    const bool expect_file_for_log_level = true;
    simple_asserts(ro, expect_file_for_log_level);

    print_effective(ro);

    std::cout << "\nMSG5_Config_Tests done.\n";
    return 0;
}
