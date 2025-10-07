#pragma once
//
// MSG5_Config — базовый класс для источников опций.
// Берёт на себя общий контракт: id/kind, no-throw fetch(), подписки на логи.
// Наследнику остаётся реализовать только fetch_impl() (и опц. prepare()).
//

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <string_view>

#include "msg5/config/logging/Logger.h"
//#include "msg5/config/OptionsSourceTypes.h"
#include "msg5/config/source/IOptionsSource.h"

namespace msg5::config {


    /// Базовый класс-скелет. Не тянет реализацию провайдеров — только общая механика.
    class SourceBase : public IOptionsSource {
    public:
        explicit SourceBase(ProviderClass k, std::string id)
            : kind_(k), id_(std::move(id)), logger_(id_, LogLevel::Info) {}

        ~SourceBase() override = default;

        // IOptionsSource
        ProviderClass   kind() const override { return kind_; }
        std::string  id()   const override { return id_; }

        // no-throw обёртка: ловит любые исключения и конвертирует их в ERROR-событие.
        FetchResult fetch(const CommandSpec& spec) noexcept final;

        void set_min_level(LogLevel l) noexcept override { logger_.set_min(l); }
        void subscribe(LogSink s) override { logger_.subscribe(std::move(s)); }

    protected:
        // Реализация источника (наследник обязан определить)
        [[nodiscard]] virtual FetchResult fetch_impl(const CommandSpec& spec) noexcept = 0;

        // Проксируем в общий Logger
        void emit(LogLevel lvl, std::string_view code, std::string_view msg) const {
            logger_.emit(lvl, code, msg);
        }

    private:
        ProviderClass      kind_;
        std::string        id_;
        Logger             logger_;
    };

} // namespace msg5::config
