#include "pch.h"
#include "EnvSource.h"

#include "msg5/config/CommandSpec.h"  // CommandSpec, OptionSpec
#include "../util/env_utils.h"                // env_exists, safe_getenv, to_env_key

#include <utility>

namespace msg5::config {

    // --- helpers ---------------------------------------------------------------

    static std::string mask_secret(std::string_view s) {
        // маскируем, но оставим длину для отладки
        return s.empty() ? std::string{} : std::string(s.size(), '*');
    }

    // --- lifecycle -------------------------------------------------------------

    void EnvSource::prepare(const CommandSpec& spec) noexcept {
        explicit_env_to_key_.clear();
        key_to_prefixed_env_.clear();
        key_flags_.clear();

        explicit_env_to_key_.reserve(explicit_env_to_key_.size() + spec.options.size() * 2);
        key_to_prefixed_env_.reserve(key_to_prefixed_env_.size() + spec.options.size());
        key_flags_.reserve(key_flags_.size() + spec.options.size());

        // Проходим по всем опциям спецификации и кэшируем:
        // 1) явные имена переменных окружения (env_names -> opt.key)
        // 2) предсчитанное имя PREFIX + TO_ENV_KEY(opt.key)
        // 3) флаг секретности (если поле есть; иначе false)
        for (const auto& opt : spec.options) {
            // 1) Явные ENV-имена
            if (!opt.env_names.empty()) {
                for (const auto& name : opt.env_names) {
                    if (!name.empty()) {
                        explicit_env_to_key_.emplace(name, opt.key);
                    }
                }
            }

            // 2) PREFIX + normalized key
            if (!opt.key.empty() && !prefix_.empty()) {
                key_to_prefixed_env_.emplace(opt.key, prefix_ + to_env_key(opt.key));
            }

            // 3) Флаги (для секретности и др.)
            key_flags_.emplace(opt.key, opt.flags);
        }
    }

    // --- fetching --------------------------------------------------------------

    FetchResult EnvSource::fetch_impl(const CommandSpec& spec) noexcept {
        FetchResult out;

        // 1) Сначала пробуем явные env_names
        //    Соберём найденные значения сразу, чтобы последующий шаг не перетёр их.
        for (const auto& [env_name, key] : explicit_env_to_key_) {
            if (env_exists(env_name.c_str())) {
                std::string value = safe_getenv(env_name.c_str());
                out.kv.emplace(key, value);

                const bool secret = ((key_flags_.count(key) ? key_flags_.at(key) : 0) & OptSecret) != 0;
                emit(LogLevel::Info,
                    "ENV_FOUND_EXPLICIT",
                    std::string(env_name)
                    + " -> " + key
                    + " = " + (secret ? mask_secret(value) : value));
            }
        }

        // 2) Затем — PREFIX + normalized(key), но только если ключ ещё не установлен
        for (const auto& opt : spec.options) {
            // пропустим, если уже выставили по explicit env_name
            if (out.kv.find(opt.key) != out.kv.end())
                continue;

            auto it = key_to_prefixed_env_.find(opt.key);
            if (it == key_to_prefixed_env_.end())
                continue; // нет предсчитанного имени (пустой prefix_ или пустой key)

            const std::string& env_name = it->second;
            if (env_name.empty())
                continue;

            if (env_exists(env_name.c_str())) {
                std::string value = safe_getenv(env_name.c_str());
                out.kv.emplace(opt.key, value);

                const bool secret = ((key_flags_.count(opt.key) ? key_flags_.at(opt.key) : 0) & OptSecret) != 0;
                emit(LogLevel::Info,
                    "ENV_FOUND_PREFIXED",
                    env_name
                    + " -> " + opt.key
                    + " = " + (secret ? mask_secret(value) : value));
            }
            else {
                // Информируем (низкий уровень), что переменная не найдена — это не ошибка.
                emit(LogLevel::Debug, "ENV_NOT_FOUND", env_name);
            }
        }

        // Никаких исключений наружу; всё, что пошло не так, уже отражено в логах.
        return out;
    }

} // namespace msg5::config
