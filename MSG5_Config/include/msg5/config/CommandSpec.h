#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace msg5::config {

    enum class OptionType { String, Path, Int, Bool, Enum };

    enum OptionFlags : unsigned {
        OptNone = 0,
        OptSecret = 1 << 0, // не печатать значение, маскировать ввод
    };

    struct OptionSpec {
        // Каноническое имя: напр., "db.host", "meta.dir"
        std::string key;

        OptionType  type{ OptionType::String };
        bool        required{ false };
        unsigned    flags{ OptNone };

        // Где искать
        std::vector<std::string> cli_flags;  // {"--db-host", "-H"}
        std::vector<std::string> env_names;  // {"MSG5_DB_HOST"}
        std::string              json_path;  // "db.host"

        // Для интерактива
        std::string prompt;                  // "DB host: "

        // Для enum
        std::vector<std::string> enum_values;

        // Дефолт (если ни один источник не дал значение)
        std::optional<std::string> default_value;
    };

    struct CommandSpec {
        std::string name;                // "validate" / "create-database" / "apply-meta-structure" / "run" ...
        std::vector<OptionSpec> options; // перечень параметров для этой команды
    };

} // namespace msg5::config
