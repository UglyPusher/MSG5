#include "pch.h"

#include "msg5/config/source/SourceBase.h"
#include <string_view>
#include <exception>

namespace msg5::config {

    FetchResult SourceBase::fetch(const CommandSpec& spec) noexcept {
        try {
            auto r = fetch_impl(spec);
            // если провайдер сам не проставил источник - задаём по механизму адаптера
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
        return FetchResult{}; // «всегда продолжаем»: на ошибке отдаём пустой результат
    }

} // namespace msg5::config
