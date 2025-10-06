#include "pch.h"
#include "FileSource.h"
#include "../util/fs_utils.h"
#include "../util/json_utils.h"
#include "msg5/config/CommandSpec.h"
#include <string_view>

#include <iostream>
#include <filesystem>

namespace msg5::config {

    // Возвращает "очищенное" превью для логов (байтовая длина, без анализа UTF-8)
    static std::string sanitize_preview(std::string_view s, std::size_t max_len = 160) noexcept {
        const bool need_ellipsis = s.size() > max_len;
        if (s.size() > max_len) s = s.substr(0, max_len);

        std::string out;
        out.reserve(s.size());

        // 1) Заменяем ASCII-контрольные символы на пробелы
        for (unsigned char c : s) {
            if (c < 0x20 || c == 0x7F) {
                out.push_back(' ');
            }
            else {
                out.push_back(static_cast<char>(c));
            }
        }

        // 2) Сжимаем повторы пробелов (включая табы/переносы, которые уже стали пробелами)
        std::string compact;
        compact.reserve(out.size());
        bool prev_space = false;
        for (char ch : out) {
            const bool is_space = (ch == ' ');
            if (is_space) {
                if (!prev_space) compact.push_back(' ');
            }
            else {
                compact.push_back(ch);
            }
            prev_space = is_space;
        }

        // 3) Трим по краям
        auto l = compact.find_first_not_of(' ');
        auto r = compact.find_last_not_of(' ');
        std::string trimmed = (l == std::string::npos) ? std::string{} : compact.substr(l, r - l + 1);

        // 4) Добавляем многоточие, если исходник длиннее max_len
        if (need_ellipsis) trimmed += "…";
        return trimmed;
    }

    // По контракту FileSource «тупой»: путь задаётся извне и не подбирается.
    FileSource::FileSource(std::filesystem::path path, LogLevel min) noexcept
        : SourceBase(ProviderClass::File, std::string("file:") + path.string())
        , path_(std::move(path)) {
        set_min_level(min);
    }

    void FileSource::prepare(const CommandSpec& /*spec*/) noexcept {
        emit(LogLevel::Debug, "FileSource::prepare", "Run");
        // Ничего не готовим: просто читаем один JSON-файл без магии.
    }

    // SourceBase::fetch(no-throw) вызывает это место.
    FetchResult FileSource::fetch_impl(const CommandSpec & spec) noexcept {
        FetchResult out;

        const std::string path = path_.string();
        emit(LogLevel::Debug, "FileSource::fetch_impl run", path);
        if (!std::filesystem::exists(path_)) {
            emit(LogLevel::Warn, "FILE_NOT_FOUND", path);
            return out;
        }
        else { 
            emit(LogLevel::Debug, "FILE_IS_FOUND", path);
        }
        
        // 1) exists/open/read — ошибки только в лог, наружу не бросаем
        /*if (!fs::file_exists(path)) {
            emit(LogLevel::Warn, "FILE_NOT_FOUND", path);
            return out;
        }*/
        
        const auto text = fs::read_all_text(path);
        if (text.empty()) {
            emit(LogLevel::Warn, "FILE_EMPTY", path);
            return out;
        }
        emit(LogLevel::Info, "FILE_READ_OK", path);

        {
            //std::string head = text.substr(0, std::min<size_t>(text.size(), 160));
            const auto head = sanitize_preview(text, 160);
            emit(LogLevel::Debug, "FILE_DEBUG_TEXT",
                "size=" + std::to_string(text.size()) + " head=" + head);
        }

        
        // 2) parse (плоский JSON) — используем вашу утилиту
        //    Формат: {"k":"v","n":123,"b":true} -> map<string,string>
        //    null пропускается.
        //    NB: это не полноценный парсер JSON, но нам хватает для плоских ключей.
        const auto flat = json::parse_flat_object(text);
        
        if (flat.empty()) {
            emit(LogLevel::Warn, "FILE_PARSE_EMPTY", path);
            return out;
        }
        else
        {
            emit(LogLevel::Debug, "FILE_PARSED", path);
        }

        {
            std::ostringstream os;
            os << "keys={";
            bool first = true;
            for (const auto& [k, v] : flat) {
                if (!first) os << ", ";
                first = false;
                os << k;
            }
            os << "} count=" << flat.size();
            emit(LogLevel::Debug, "FILE_DEBUG_FLAT_KEYS", os.str());
        }
        
        // 3) Извлекаем только те ключи, что описаны в CommandSpec.
        //    Приоритет: если у опции есть json_path — ищем его;
        //    иначе используем её key как имя плоского поля.
        for (const auto& opt : spec.options) {
            const std::string & want = !opt.json_path.empty() ? opt.json_path : opt.key;
            if (want.empty()) continue;
            auto it = flat.find(want);
            if (it != flat.end()) {
                // Записываем в итоговый ключ конфигурации (opt.key),
                // значение уже приведено к строке утилитой parse_flat_object.
                out.kv.emplace(opt.key, it->second);
                emit(LogLevel::Debug, "FILE_DEBUG_MATCH", "want=" + want + " -> key=" + opt.key);
            }
            else {
                emit(LogLevel::Debug, "FILE_DEBUG_MISS", "want=" + want);
            }
        }

        //emit(LogLevel::Info, "FILE_READ_OK", path);
        emit(LogLevel::Info, "FILE_KEYS_EMITTED", std::to_string(out.kv.size()));
        return out;
    }
    

}
