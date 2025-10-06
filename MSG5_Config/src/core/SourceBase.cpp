#include "pch.h"
//
// –еализаци€ базового класса источника
//
#include "msg5/config/source/SourceBase.h"

#include <exception>

namespace msg5::config {

    static inline int rank(LogLevel l) noexcept {
        switch (l) {
        case LogLevel::Error: return 0;
        case LogLevel::Warn:  return 1;
        case LogLevel::Info:  return 2;
        case LogLevel::Debug: return 3;
        case LogLevel::Trace: return 4;
        }
        return 2;
    }

    void SourceBase::set_min_level(LogLevel lvl) noexcept {
        min_level_.store(lvl, std::memory_order_relaxed);
    }
    
    LogLevel SourceBase::min_level() const noexcept {
        return min_level_.load(std::memory_order_relaxed);
    }

    void SourceBase::emit(LogLevel lvl, std::string_view code, std::string_view message) const {
        if (sinks_.empty()) return;
        if (rank(lvl) > rank(min_level_.load(std::memory_order_relaxed))) return;
        LogEvent ev;
        ev.level = lvl;
        ev.code = std::string(code);
        ev.message = std::string(message);
        ev.source_id = id_;
        for (const auto& s : sinks_) {
            if (s) s(ev);
        }
    }

    FetchResult SourceBase::fetch(const CommandSpec& spec) noexcept {
        try {
            auto r = fetch_impl(spec);
            // если провайдер сам не проставил источник Ч задаЄм по механизму адаптера
            if (r.source == ValueSource::Default) {
                r.source = value_source_of(kind_);
            }
            return r;
        }
        catch (const std::exception& e) {
            emit(LogLevel::Error, "EXCEPTION", e.what());
        }
        catch (...) {
            emit(LogLevel::Error, "EXCEPTION", "unknown exception");
        }
        return FetchResult{}; // Ђвсегда продолжаемї: на ошибке отдаЄм пустой результат
    }

} // namespace msg5::config
