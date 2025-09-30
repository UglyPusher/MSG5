#include <iostream>
#include <string>
#include <vector>
#include <cassert>

#include "msg5/config/Resolver.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h" 
#include "msg5/config/OptionsSourceTypes.h" // Origin, OptSecret
#include "msg5/config/Sources.h"            // makeArgsSource/makeFileSource/makeEnvSource/makePromptSource

using namespace msg5::config;

// Мини-утилита печати результата
static void print_effective(const ResolvedOptions& ro) {
    std::cout << "=== Effective config ===\n";
    for (const auto& kv : ro.values) {
        const std::string& k = kv.first;
        const std::string& v = kv.second;
        Origin o = Origin::Default;
        if (auto it = ro.origins.find(k); it != ro.origins.end()) {
            o = it->second;
        }
        const char* oname =
            (o == Origin::Cli) ? "Cli" :
            (o == Origin::ConfigFile) ? "File" :
            (o == Origin::Env) ? "Env" :
            (o == Origin::Stdin) ? "Stdin" : "Default";
        std::cout << "  " << k << " = " << v << "  [" << oname << "]\n";
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

    std::vector<IOptionsSourcePtr> sources;
    sources.emplace_back(makeArgsSource(argc, argv));
    sources.emplace_back(makeFileSource("config/user.json")); // можно убрать/переименовать для проверки FILE_NOT_FOUND
    sources.emplace_back(makeEnvSource("MSG5_"));
    sources.emplace_back(makePromptSource());

    Resolver r(std::move(sources));
    return r.resolve(spec);
}

// Небольшие sanity-проверки порядка слоёв (без тяжёлого фреймворка)
static void simple_asserts(const ResolvedOptions& ro, bool expect_file_for_log_level) {
    // Если в файле присутствовал log.level — ожидаем Origin::File
    if (expect_file_for_log_level) {
        auto it = ro.origins.find("log.level");
        assert(it != ro.origins.end() && "log.level must exist");
        assert(it->second == Origin::File && "log.level origin must be File");
    }

    // Если CLI передан как --db.host=..., то он должен быть сильнее (ранний)
    // Здесь просто проверяем наличие значения, Origin проверяется вручную при запуске с CLI.
    // Пример запуска: ... --db.host=cli.local
    (void)ro; // чтобы не ругался компилятор, если не используем
}

int main(int argc, const char* argv[]) {
    // Сценарий: смотрим, что собралось, и печатаем
    const ResolvedOptions ro = run_basic(argc, argv);

    // Считаем, что в репозитории присутствует пример config/user.json с полем log.level,
    // тогда ожидаем Origin::File для log.level. При необходимости выставьте флаг в false.
    const bool expect_file_for_log_level = true;
    simple_asserts(ro, expect_file_for_log_level);

    print_effective(ro);

    // Успех — код 0
    std::cout << "\nMSG5_Config_Tests done.\n";
    return 0;
}
