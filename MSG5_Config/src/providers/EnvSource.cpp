#include "pch.h"
#include "EnvSource.h"
#include "src/util/env_utils.h"
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>

namespace msg5::config {

    FetchResult EnvSource::fetch(const CommandSpec& spec) {
        FetchResult out;

        for (const auto& opt : spec.options) {
            // 👇 ВАЖНО: подставь правильное поле из OptionSpec с именем ключа.
            // Часто это opt.key или opt.name. Если у тебя иначе — замени на своё.
            const std::string& key = opt.key;   // <-- если у тебя 'name', то: opt.name

            bool set = false;

            // 1) Явные имена переменных окружения у опции
            for (const auto& envName : opt.env_names) {
                if (envName.empty()) continue;
                if (env_exists(envName.c_str())) {
                    out.kv[key] = safe_getenv(envName); // сохраняем даже пустую строку
                    set = true;
                    break;
                }
            }
            if (set) continue;

            // 2) Нормализованное имя из самого ключа (без префикса)
            std::string envKey = to_env_key(key);
            if (env_exists(envKey.c_str())) {
                out.kv[key] = safe_getenv(envKey);
            }
        }

        return out;
    }


} // namespace msg5::config
