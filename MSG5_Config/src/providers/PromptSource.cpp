#pragma once
#include "pch.h"
#include "PromptSource.h"
#include "../util/json_utils.h"
#include "msg5/config/CommandSpec.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
# include <io.h>     // _isatty, _fileno
# include <conio.h>  // _getch (для скрытого ввода)
#else
# include <unistd.h> // isatty, fileno
# include <termios.h>
#endif


namespace msg5::config {
    static bool stdin_is_tty() {
#ifdef _WIN32
        return _isatty(_fileno(stdin)) != 0;
#else
        return isatty(fileno(stdin)) != 0;
#endif
    }

    static std::string read_line_visible(const std::string& prompt) {
        std::string s;
        std::cout << prompt << std::flush;
        std::getline(std::cin, s);
        return s;
    }

    static std::string read_line_secret(const std::string& prompt) {
        std::cout << prompt << std::flush;
#ifdef _WIN32
        std::string s;
        for (;;) {
            int ch = _getch();
            if (ch == 13 || ch == 10) { std::cout << "\n"; break; } // Enter
            if (ch == 3) { std::cout << "\n"; return {}; }          // Ctrl+C
            if (ch == 8) { if (!s.empty()) s.pop_back(); continue; } // Backspace
            if (ch >= 32 && ch <= 126) s.push_back(static_cast<char>(ch));
        }
        return s;
#else
        termios old{};
        if (tcgetattr(fileno(stdin), &old) == 0) {
            termios t = old; t.c_lflag &= ~ECHO; tcsetattr(fileno(stdin), TCSAFLUSH, &t);
            std::string s; std::getline(std::cin, s); std::cout << "\n";
            tcsetattr(fileno(stdin), TCSAFLUSH, &old);
            return s;
        }
        std::string s; std::getline(std::cin, s); std::cout << "\n"; return s;
#endif
    }


    // Вспомогательная маскировка
    static std::string mask_secret(const std::string & s) {
        return s.empty() ? std::string{} : std::string(s.size(), '*');
    }
    
    FetchResult PromptSource::fetch_impl(const CommandSpec & spec) noexcept {
        FetchResult out;

        // Если stdin — консоль (TTY)
        if (stdin_is_tty()) {
            if (!interactive_) {
                emit(LogLevel::Info, "STDIN_NOT_PIPE", "interactive console");
                return out; // не блокируемся без интерактива
            }
            // Интерактив: спрашиваем только ключи из spec (Resolver уже фильтрует «дыры»)
            for (const auto& opt : spec.options) {
                const std::string name = !opt.json_path.empty() ? opt.json_path : opt.key;
                if (name.empty()) continue;
                const bool secret = ((opt.flags & OptSecret) != 0);
                const std::string prompt = name + (secret ? " (secret, Enter=skip)" : " (Enter=skip)") + " : ";
                std::string val = secret ? read_line_secret(prompt) : read_line_visible(prompt);
                if (!val.empty()) {
                    out.kv.emplace(opt.key, val);
                    // лёгкая телеметрия: показываем, что приняли значение
                    emit(LogLevel::Info, "STDIN_PROMPT_VALUE",
                        name + " -> " + opt.key + (secret ? " = ****" : " = " + val));
                }
            }
            return out;
        }
        
        // PIPE/FILE: читаем всё до EOF
        std::ostringstream ss; ss << std::cin.rdbuf();
        const std::string text = ss.str();
        if (text.empty()) { emit(LogLevel::Info, "STDIN_EMPTY", "no data"); return out; }
        
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
