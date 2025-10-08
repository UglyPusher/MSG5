#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <optional>

#include "msg5/config/OptionsSourceTypes.h" // ValueSource

namespace msg5::config {

    // ВАЖНО: форвард-декларация на верхнем уровне (не как вложенный тип)
    struct CommandSpec;

    struct ResolvedOptions {
        std::string command;
        std::unordered_map<std::string, std::string> values;
        std::unordered_map<std::string, ValueSource> value_sources;

        std::filesystem::path config_path;
        std::filesystem::path config_dir;

        // Ошибка одной опции (key + message)
        struct ValidationError {
            std::string key;
            std::string message;
        };
        using ValidationResult = std::vector<ValidationError>;

        bool used_env{ false };
        bool used_stdin{ false };

        [[nodiscard]] bool has(std::string_view key) const;
        [[nodiscard]] ValueSource origin(std::string_view key) const;
        [[nodiscard]] std::string get_str(std::string_view key, std::string_view def = "") const;
        [[nodiscard]] bool get_bool(std::string_view key, bool def = false) const;
        [[nodiscard]] int get_int(std::string_view key, int def = 0) const;
        [[nodiscard]] std::filesystem::path get_path(std::string_view key,
            std::filesystem::path def = {}) const;

        // Валидация против CommandSpec:
        //  - НЕ бросает исключений;
        // - возвращает true, если ошибок нет;
        // - при false заполняет 'out' списком ошибок.
        bool validate(const struct CommandSpec& spec,
            ValidationResult & out) const;
    };

} // namespace msg5::config

