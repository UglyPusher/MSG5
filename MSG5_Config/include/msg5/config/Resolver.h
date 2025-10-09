#pragma once
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <string_view>

#include "msg5/config/logging/Logger.h"
#include "msg5/config/ResolvedOptions.h"

namespace msg5::config {
    struct CommandSpec; // объявление вперед

    struct IOptionsSource;
    using IOptionsSourcePtr = std::unique_ptr<IOptionsSource>;

    struct ResolveParams {
        bool apply_defaults = false;                       // подставить default_value из spec
        bool validate = false;                       // запустить validate()
        bool log_validation = true;                        // логировать VALIDATION_* (если validate=true)
        ResolvedOptions::ValidationResult* out_errors = nullptr; // куда сложить ошибки (опц.)
    };

    class Resolver {
    public:
        explicit Resolver(std::vector<IOptionsSourcePtr> sources) noexcept;
        
        // Совместимость: старый интерфейс — просто сбор (без дефолтов/валидации)
        [[nodiscard]] ResolvedOptions resolve(const CommandSpec& spec);

        // Параметризуемый пайплайн: сбор → (опц.) defaults → (опц.) validate (+лог)
        [[nodiscard]] ResolvedOptions resolve(const CommandSpec& spec, const ResolveParams& params);

        // Подписка на события резолвера (тот же формат, что у SourceBase/Logger)
        using LogSink = std::function<void(const LogEvent&)>;
        void subscribe(LogSink s) { sinks_.push_back(std::move(s)); }
        void set_min_level(LogLevel lvl) noexcept { logger_.set_min(lvl); }
        LogLevel min_level() const noexcept { return logger_.min(); }
    private:
        // 0-1) подготовка источников и сбор с мёрджем по приоритетам
        [[nodiscard]] ResolvedOptions collect_(const CommandSpec& spec);

        // 2) (опц.) применить дефолты и залогировать факт
        void maybe_apply_defaults_(ResolvedOptions& ro, const CommandSpec& spec,
            const ResolveParams& p);

        // 3) (опц.) валидация и (опц.) логирование результата
        void maybe_validate_and_log_(const ResolvedOptions& ro, const CommandSpec& spec,
            const ResolveParams& p);

        // Локальный лог + ретрансляция подписчикам
        void emit(LogLevel level, std::string_view code, std::string_view msg) const;

    private:
        std::vector<IOptionsSourcePtr> sources_;
        
        Logger                logger_{ "resolver", LogLevel::Info };
        std::vector<LogSink>  sinks_;
    };
}
