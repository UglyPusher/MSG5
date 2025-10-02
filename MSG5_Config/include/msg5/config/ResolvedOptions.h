#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include "msg5/config/OptionsSourceTypes.h" // ValueSource

namespace msg5::config {

    // ВАЖНО: форвард-декларация на верхнем уровне (не как вложенный тип)
    struct CommandSpec;

    struct ResolvedOptions {
        std::string command;
        std::unordered_map<std::string, std::string> values;
        std::unordered_map<std::string, ValueSource>      value_sources;

        std::filesystem::path config_path;
        std::filesystem::path config_dir;

        bool used_env{ false };
        bool used_stdin{ false };

        bool has(std::string_view key) const {
            return values.find(std::string(key)) != values.end();
        }

        ValueSource origin(std::string_view key) const {
            auto it = value_sources.find(std::string(key));
            return it == value_sources.end() ? ValueSource::Default : it->second;
        }

        std::string get_str(std::string_view key, std::string_view def = "") const {
            auto it = values.find(std::string(key));
            return it == values.end() ? std::string(def) : it->second;
        }

        static bool parse_bool(std::string v, bool def) {
            std::transform(v.begin(), v.end(), v.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (v == "1" || v == "true" || v == "yes" || v == "on")  return true;
            if (v == "0" || v == "false" || v == "no" || v == "off") return false;
            return def;
        }


        bool get_bool(std::string_view key, bool def = false) const {
            auto it = values.find(std::string(key));
            return it == values.end() ? def : parse_bool(it->second, def);
        }

        int get_int(std::string_view key, int def = 0) const {
            auto it = values.find(std::string(key));
            if (it == values.end()) return def;
            try { return std::stoi(it->second); }
            catch (...) { throw std::runtime_error("Option '" + std::string(key) + "' is not a valid integer"); }
        }

        std::filesystem::path get_path(std::string_view key,
            std::filesystem::path def = {}) const {
            auto it = values.find(std::string(key));
            if (it == values.end()) return def;
            std::filesystem::path p = it->second;
            if (p.is_relative() && !config_dir.empty()) p = config_dir / p;
            return p;
        }

        // Базовая валидация по спецификации (обязательность и enum-значения).
        void validate(const CommandSpec& spec) const;
    };

} // namespace msg5::config

