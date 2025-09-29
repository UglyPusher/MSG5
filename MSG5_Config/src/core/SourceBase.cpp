#include "pch.h"
//
// Реализация базового класса источника
//
#include "msg5/config/source/SourceBase.h"

#include <exception>

namespace msg5::config {

    void SourceBase::emit(LogLevel lvl, std::string_view code, std::string_view message) const {
        if (sinks_.empty()) return;
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
            return fetch_impl(spec);
        }
        catch (const std::exception& e) {
            emit(LogLevel::Error, "EXCEPTION", e.what());
        }
        catch (...) {
            emit(LogLevel::Error, "EXCEPTION", "unknown exception");
        }
        return FetchResult{}; // «всегда продолжаем»: на ошибке отдаём пустой результат
    }

} // namespace msg5::config
