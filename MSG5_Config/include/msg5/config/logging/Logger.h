#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace msg5::config {

    enum class LogLevel : unsigned char { Error, Warn, Info, Debug, Trace };

    struct LogEvent {
        LogLevel     level{};
        std::string  source_id;  // "resolver", "cli", "env:MSG5_", "file:/path.json", ...
        std::string  code;       // "ASK","GOT","ENV_NOT_FOUND","FILE_READ_OK", ...
        std::string  message;    // короткое описание (k=v, counts и т.п.)
    };

    using LogSink = std::function<void(const LogEvent&)>;

    // Единый «ранг» уровня логов (чем больше — тем «болтливее»)
    constexpr int log_rank(LogLevel l) noexcept {
        switch (l) {
        case LogLevel::Error: return 0;
        case LogLevel::Warn:  return 1;
        case LogLevel::Info:  return 2;
        case LogLevel::Debug: return 3;
        case LogLevel::Trace: return 4;
        }
        return 2; // по умолчанию Info
    }

    // Небольшой, самостоятельный логгер для композиции в классах
    class Logger {
    public:
        explicit Logger(std::string id = {}, LogLevel min = LogLevel::Info) noexcept;

        // Идентификатор источника (попадает в LogEvent::source_id)
        void set_id(std::string id) noexcept;
        [[nodiscard]] const std::string& id() const noexcept { return id_; }

        // Уровень отсечения
        void set_min(LogLevel lvl) noexcept;
        [[nodiscard]] LogLevel min() const noexcept { return min_.load(std::memory_order_relaxed); }

        // Подписка на события (по договорённости — добавляем до старта работы)
        void subscribe(LogSink s);

        // Отправить событие
        void emit(LogLevel lvl, std::string_view code, std::string_view msg) const;

    private:
        std::string              id_;
        std::atomic<LogLevel>    min_;
        std::vector<LogSink>     sinks_;
    };

} // namespace msg5::config
