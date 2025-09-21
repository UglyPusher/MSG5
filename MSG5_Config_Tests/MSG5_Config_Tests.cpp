#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/Resolver.h"
#include "msg5/config/errors.h"

// провайдеры из проекта MSG5_Config
#include "src/providers/ArgsSource.h"
#include "src/providers/EnvSource.h"

using namespace msg5::config;

static void print_effective(const ResolvedOptions& ro) {
    // Сортируем ключи для стабильного вывода
    std::vector<std::string> keys;
    keys.reserve(ro.values.size());
    for (const auto& kv : ro.values) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());

    std::cout << "Effective configuration (" << keys.size() << " items)\n";
    for (const auto& k : keys) {
        const auto& v = ro.values.at(k);
        auto o = ro.origin(k);
        std::cout << "  " << k << " = " << v
            << "  (origin=" << to_string(o) << ")\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        // 1) Описываем команду и опции
        CommandSpec spec;
        spec.name = "msg5-config-tests";
        spec.options.clear();
        spec.options.reserve(3);

        // db.host
        {
            OptionSpec o;
            o.key = "db.host";
            o.type = OptionType::String;
            o.required = true;
            o.flags = OptNone; // поставить OptSecret при необходимости
            o.json_path = "db.host";           // не обязательно, но корректно
            o.env_names = { "MSG5_DB_HOST" };  // удобно для EnvSource
            o.cli_flags = { "--db.host" };     // для парсера CLI
            spec.options.push_back(std::move(o));
        }

        // db.port
        {
            OptionSpec o;
            o.key = "db.port";
            o.type = OptionType::Int;
            o.required = true;
            o.flags = OptNone;
            o.json_path = "db.port";
            o.env_names = { "MSG5_DB_PORT" };
            o.cli_flags = { "--db.port" };
            spec.options.push_back(std::move(o));
        }

        // log.level
        {
            OptionSpec o;
            o.key = "log.level";
            o.type = OptionType::Enum;
            o.required = false;
            o.flags = OptNone;
            o.json_path = "log.level";
            o.env_names = { "MSG5_LOG_LEVEL" };
            o.cli_flags = { "--log.level" };
            o.enum_values = { "debug", "info", "warn", "error" };
            o.default_value = std::string("info");
            spec.options.push_back(std::move(o));
        }

        // 2) Источники (приоритет слева направо): CLI > ENV
        std::vector<IOptionsSourcePtr> sources;
        sources.emplace_back(std::make_unique<ArgsSource>(argc, argv));
        sources.emplace_back(std::make_unique<EnvSource>());

        // 3) Резолвим и базово валидируем по спецификации
        Resolver resolver(std::move(sources));
        ResolvedOptions ro = resolver.resolve(spec);
        ro.validate(spec);

        // 4) Печатаем «эффективный» конфиг
        print_effective(ro);
        return EXIT_OK;
    }
    catch (const user_error& e) {
        std::cerr << "User error: " << e.what() << "\n";
        return EXIT_USER_ERR;
    }
    catch (const system_error& e) {
        std::cerr << "System error: " << e.what() << "\n";
        return EXIT_SYS_ERR;
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return EXIT_UNKNOWN_ERR;
    }
}
