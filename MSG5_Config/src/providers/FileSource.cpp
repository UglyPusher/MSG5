#include "pch.h"
#include "FileSource.h"
#include "../util/fs_utils.h"
#include "../util/json_utils.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {

    // По контракту FileSource «тупой»: путь задаётся извне и не подбирается.
    FileSource::FileSource(std::filesystem::path path)
        : SourceBase(ProviderClass::File, std::string("file:") + path.string())
        , path_(std::move(path)) {}

    void FileSource::prepare(const CommandSpec& /*spec*/) {
        // Ничего не готовим: просто читаем один JSON-файл без магии.
    }

    // SourceBase::fetch(no-throw) вызывает это место.
    FetchResult FileSource::fetch_impl(const CommandSpec & spec) {
        FetchResult out;

        const std::string path = path_.string();
        
        // 1) exists/open/read — ошибки только в лог, наружу не бросаем
        if (!fs::file_exists(path)) {
            emit(LogLevel::Warn, "FILE_NOT_FOUND", path);
            return out;
        }
        
        const std::string text = fs::read_all_text(path);
        if (text.empty()) {
            // пустой файл — это странно для конфигурации, но продолжаем
            emit(LogLevel::Warn, "FILE_EMPTY", path);
            return out;
        }
        
        // 2) parse (плоский JSON) — используем вашу утилиту
        //    Формат: {"k":"v","n":123,"b":true} -> map<string,string>
        //    null пропускается.
        //    NB: это не полноценный парсер JSON, но нам хватает для плоских ключей.
        const auto flat = json::parse_flat_object(text);
        
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
            }
        }
        emit(LogLevel::Info, "FILE_READ_OK", path);
        return out;
    }

}
