#include "pch.h"
#include "PromptSource.h"
#include "../util/json_utils.h"
#include <iostream>
#include <sstream>

namespace msg5::config {

    FetchResult PromptSource::fetch(const CommandSpec& /*spec*/) {
        FetchResult out;

        // Если stdin пуст (не pipe) — многие компиляторы/консоль дают сразу eof/peek()==EOF.
        // Считываем всё до EOF.
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        auto text = ss.str();
        if (text.empty()) return out;

        // Ожидаем плоский JSON-объект: {"k":"v", "n":123, "b":true}
        auto flat = json::parse_flat_object(text);
        for (auto& kvp : flat) {
            out.kv[kvp.first] = kvp.second;
        }
        return out;
    }

} // namespace msg5::config
