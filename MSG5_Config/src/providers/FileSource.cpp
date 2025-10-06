#include "pch.h"
#include "FileSource.h"
#include "../util/fs_utils.h"
#include "../util/json_utils.h"
#include "msg5/config/CommandSpec.h"

#include <iostream>
#include <filesystem>

namespace msg5::config {

    // По контракту FileSource «тупой»: путь задаётся извне и не подбирается.
    FileSource::FileSource(std::filesystem::path path)
        : SourceBase(ProviderClass::File, std::string("file:") + path.string())
        , path_(std::move(path)) {}

    void FileSource::prepare(const CommandSpec& /*spec*/) {
        emit(LogLevel::Debug, "FileSource::prepare", "Run");
        // Ничего не готовим: просто читаем один JSON-файл без магии.
    }

    // SourceBase::fetch(no-throw) вызывает это место.
    FetchResult FileSource::fetch_impl(const CommandSpec & spec) {
        FetchResult out;

        const std::string path = path_.string();
        emit(LogLevel::Info, "FileSource::fetch_impl run", path);
        if (!std::filesystem::exists(path)) {
            emit(LogLevel::Warn, "FILE_NOT_FOUND", path);
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
            std::string head = text.substr(0, std::min<size_t>(text.size(), 160));
            emit(LogLevel::Debug, "FILE_DEBUG_TEXT",
                +"size=" + std::to_string(text.size()) + " head=" + head);
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
