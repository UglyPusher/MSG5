#pragma once
//
// MSG5_Config — базовый класс для источников опций.
// Берёт на себя общий контракт: id/kind, no-throw fetch(), подписки на логи.
// Наследнику остаётся реализовать только fetch_impl() (и опц. prepare()).
//

#include <string>
#include <vector>
#include <functional>
#include "msg5/config/OptionsSourceTypes.h"
#include "msg5/config/source/IOptionsSource.h"

namespace msg5::config {

    // Уровни лог-событий источника
    enum class LogLevel : unsigned char { Error, Warn, Info, Debug, Trace };

    // Событие источника (для диагностики/телеметрии)
    struct LogEvent {
        LogLevel    level{};
        std::string code;        // короткий код: FILE_NOT_FOUND, ENV_READ_OK, CLI_FLAG_UNKNOWN, ...
        std::string message;     // человекочитаемое сообщение
        std::string source_id;   // откуда прилетело (id())
    };

    // Подписчик на события источника
    using LogSink = std::function<void(const LogEvent&)>;

    /// Базовый класс-скелет. Не тянет реализацию провайдеров — только общая механика.
    class SourceBase : public IOptionsSource {
    public:
        SourceBase(SourceKind kind, std::string id) noexcept
            : kind_(kind), id_(std::move(id)) {
        }
        ~SourceBase() override = default;

        // IOptionsSource
        SourceKind   kind() const override { return kind_; }
        std::string  id()   const override { return id_; }

        // no-throw обёртка: ловит любые исключения и конвертирует их в ERROR-событие.
        FetchResult fetch(const CommandSpec& spec) noexcept final;

        // Подписки на события (локально в источнике)
        void subscribe(LogSink sink) { sinks_.push_back(std::move(sink)); }

    protected:
        // Реализация источника (наследник обязан определить)
        virtual FetchResult fetch_impl(const CommandSpec& spec) = 0;

        // Удобный эмиттер событий
        void emit(LogLevel lvl, std::string_view code, std::string_view message) const;

    private:
        SourceKind         kind_;
        std::string        id_;
        std::vector<LogSink> sinks_;
    };

} // namespace msg5::config
