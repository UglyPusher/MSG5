#include "pch.h"
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <charconv>
#include <limits>

#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {
    
    // ---------- локальные хелперы (спрятаны в TU) ----------
    namespace {
        bool parse_bool(std::string_view v, bool def) noexcept {
            std::string s(v);
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (s == "1" || s == "true" || s == "yes" || s == "on")  return true;
            if (s == "0" || s == "false" || s == "no" || s == "off") return false;
            return def;
        }

        bool parse_int64(std::string_view s, long long& out) noexcept {
            if (s.empty()) return false;
            const char* b = s.data(); const char* e = b + s.size();
            auto r = std::from_chars(b, e, out, 10);
            return r.ec == std::errc{} && r.ptr == e;
        }
        
        bool is_bool_token(std::string_view v) noexcept {
            std::string s(v);
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return s == "1" || s == "true" || s == "yes" || s == "on" || s == "0" || s == "false" || s == "no" || s == "off";
        }

    } // namespace
    
    bool ResolvedOptions::has(std::string_view key) const {
        return values.find(std::string(key)) != values.end();
    }
    
    ValueSource ResolvedOptions::origin(std::string_view key) const {
        auto it = value_sources.find(std::string(key));
        return it == value_sources.end() ? ValueSource::Default : it->second;
    }
    
    std::string ResolvedOptions::get_str(std::string_view key, std::string_view def) const {
        auto it = values.find(std::string(key));
        return it == values.end() ? std::string(def) : it->second;
    }
    
    bool ResolvedOptions::get_bool(std::string_view key, bool def) const {
        auto it = values.find(std::string(key));
        return it == values.end() ? def : parse_bool(it->second, def);
    }
    
    int ResolvedOptions::get_int(std::string_view key, int def) const {
        auto it = values.find(std::string(key));
        if (it == values.end()) return def;
        const std::string & s = it->second;
        long long v{};
        auto r = std::from_chars(s.data(), s.data() + s.size(), v, 10);
        if (r.ec != std::errc{} || r.ptr != s.data() + s.size()) return def; // мягко возвращаем def
        if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) return def;
        return static_cast<int>(v);
    }
    
    std::filesystem::path ResolvedOptions::get_path(std::string_view key,
        std::filesystem::path def) const {
        auto it = values.find(std::string(key));
        if (it == values.end()) return def;
        std::filesystem::path p = it->second;
        if (p.is_relative() && !config_dir.empty()) p = config_dir / p;
        return p;
    }
    // ---------------------------------------------------------------
    
    bool ResolvedOptions::validate(const CommandSpec & spec,
        ValidationResult & out) const noexcept {
        out.clear();

        for (const auto& opt : spec.options) {
            const auto& key = opt.key;
            const bool has = this->has(key);
            const std::string val = has ? this->get_str(key) : std::string();

            // 1) required
            if (opt.required && !has) {
                out.push_back({ key, "required value is missing" });
                continue; // дальше для этого ключа нечего проверять
            }
            if (!has) continue; // не required и отсутствует — ок

            // 2) type checks
            switch (opt.type) {
            case OptionType::String:
                // строка всегда валидна, доп. проверки (regex) при желании позже
                break;

            case OptionType::Path:
                if (val.empty()) out.push_back({ key, "path is empty" });
                break;

            case OptionType::Int: {
                long long v{};
                if (!parse_int64(val, v)) {
                    out.push_back({ key, "expected integer" });
                }
                break;
            }

            case OptionType::Bool: {
                if (!is_bool_token(val)) {
                    out.push_back({ key, "expected boolean (true/false/1/0/yes/no/on/off)" });
                }
                break;
            }

            case OptionType::Enum: {
                if (opt.enum_values.empty()) {
                    out.push_back({ key, "enum has empty values set in spec" });
                }
                else if (std::find(opt.enum_values.begin(), opt.enum_values.end(), val) == opt.enum_values.end()) {
                    out.push_back({ key, "value '" + val + "' is not within allowed enum" });
                }
                break;
            }
            }
        }

        return out.empty();
    }

} // namespace msg5::config
