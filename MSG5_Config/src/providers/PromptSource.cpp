#include "pch.h"
#include "PromptSource.h"
#include "../util/json_utils.h"
#include "msg5/config/CommandSpec.h"
#include <iostream>
#include <sstream>

namespace msg5::config {

    // Вспомогательная маскировка
    static std::string mask_secret(const std::string & s) {
        return s.empty() ? std::string{} : std::string(s.size(), '*');
    }
    
    FetchResult PromptSource::fetch_impl(const CommandSpec & spec) {
        FetchResult out;

        // 1) Считываем всё до EOF (если stdin не подан — будет пусто)
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        const std::string text = ss.str();
        if (text.empty()) {
            emit(LogLevel::Info, "STDIN_EMPTY", "no data");
            return out;
        }
        
        // 2) Парсим плоский JSON-объект (используем ваши утилиты)
        //    Пример: {"k":"v", "n":123, "b":true} -> map<string,string>
        const auto flat = json::parse_flat_object(text);
        if (flat.empty()) {
            // parse_flat_object возвращает {} и при пустом/некорректном вводе
            emit(LogLevel::Warn, "STDIN_PARSE_ERROR", "empty or invalid flat JSON");
            return out;
        }
        
        // 3) Отдаём только ключи, которые описаны в спецификации
        for (const auto& opt : spec.options) {
            const std::string & name = !opt.json_path.empty() ? opt.json_path : opt.key;
            if (name.empty()) continue;
            auto it = flat.find(name);
            if (it == flat.end()) continue;
            
            const bool secret = ((opt.flags & OptSecret) != 0);
            out.kv.emplace(opt.key, it->second);
            emit(LogLevel::Info,
                "STDIN_KEY_ACCEPTED",
                name + " -> " + opt.key + (secret ? " = ****" : " = " + it->second));
        }
        
        emit(LogLevel::Info, "STDIN_READ_OK", "processed");
        return out;
    }

} // namespace msg5::config
