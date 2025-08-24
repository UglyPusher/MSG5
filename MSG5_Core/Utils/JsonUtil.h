#pragma once
#include <string>
#include <string_view>

namespace msg5::json {

    // Экранирует строку по JSON-правилам (без внешних кавычек)
    std::string Escape(std::string_view s);

    // Возвращает строку уже в кавычках:  "...\u000a..."
    inline std::string Quote(std::string_view s) {
        std::string out = "\"";
        out += Escape(s);
        out += "\"";
        return out;
    }

} // namespace msg5::json
