#pragma once
#include <vector>
#include <memory> // for std::unique_ptr
#include <unordered_map>
#include <string>   // std::string
#include <string_view>
#include <functional>

#include "msg5/config/logging/Logger.h"
#include "msg5/config/ResolvedOptions.h"

namespace msg5::config {
    struct CommandSpec; // объявление вперед

    struct IOptionsSource;
    using IOptionsSourcePtr = std::unique_ptr<IOptionsSource>;

    class Resolver {
    public:
        explicit Resolver(std::vector<IOptionsSourcePtr> sources) noexcept;
        
        [[nodiscard]] ResolvedOptions resolve(const CommandSpec& spec) noexcept;
        
        // Подписка на события резолвера (тот же формат, что у SourceBase/Logger)
        using LogSink = std::function<void(const LogEvent&)>;
        void subscribe(LogSink s) { sinks_.push_back(std::move(s)); }
        void set_min_level(LogLevel lvl) noexcept { logger_.set_min(lvl); }
    private:
        std::vector<IOptionsSourcePtr> sources_;
        
        // логирование
        void emit(LogLevel level, std::string_view code, std::string_view msg) const;
        Logger                logger_{ "resolver", LogLevel::Info };
        std::vector<LogSink>  sinks_;
    };
}
