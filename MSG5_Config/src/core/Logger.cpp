#include "msg5/config/logging/Logger.h"

namespace msg5::config {

    Logger::Logger(std::string id, LogLevel min) noexcept
        : id_(std::move(id)), min_(min) {
    }

    void Logger::set_id(std::string id) noexcept {
        id_ = std::move(id);
    }

    void Logger::set_min(LogLevel lvl) noexcept {
        min_.store(lvl, std::memory_order_relaxed);
    }

    void Logger::subscribe(LogSink s) {
        sinks_.push_back(std::move(s));
    }

    void Logger::emit(LogLevel lvl, std::string_view code, std::string_view msg) const {
        if (log_rank(lvl) > log_rank(min_.load(std::memory_order_relaxed))) return;

        LogEvent ev;
        ev.level = lvl;
        ev.source_id = id_;
        ev.code = std::string(code);
        ev.message = std::string(msg);

        for (const auto& sink : sinks_) {
            sink(ev);
        }
    }

} // namespace msg5::config
