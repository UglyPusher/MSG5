#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "msg5/config/source/SourceBase.h"

namespace msg5::config {

// forward-decls, чтобы не тянуть тяжёлые заголовки в .h
struct CommandSpec;
struct OptionSpec; // если нужно в prepare (необязательно, но удобно)

class EnvSource final : public SourceBase {
public:
    // Префикс задаёт приложение (например, "MSG5_"). Он не пустой.
    explicit EnvSource(std::string prefix)
        : SourceBase(ProviderClass::Env, std::string("env:") + prefix)
        , prefix_(std::move(prefix)) {}

    // Подготовка к выборке: кэшируем env_names и предрассчитанные имена вида PREFIX+TO_ENV_KEY(key)
    void prepare(const CommandSpec& spec) override;

protected:
    // Реальная работа источника (SourceBase::fetch no-throw вызовет это)
    FetchResult fetch_impl(const CommandSpec& spec) override;

private:
    // Входные параметры / кэш
    std::string prefix_; // например, "MSG5_"

    // Явные имена окружения из спецификации: ENV_NAME -> opt.key
    std::unordered_map<std::string, std::string> explicit_env_to_key_;

    // Предсчитанные имена окружения по ключам опций: opt.key -> (PREFIX + to_env_key(opt.key))
    std::unordered_map<std::string, std::string> key_to_prefixed_env_;

    // Флаги опций по ключу (для маскирования и др.)
    std::unordered_map<std::string, int> key_flags_;
};

} // namespace msg5::config
