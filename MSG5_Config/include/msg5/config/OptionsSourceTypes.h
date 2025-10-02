#pragma once
//
// MSG5_Config — базовые типы источников (не тянут интерфейс).
// Подходит для инклуда из основной аппки.
//

#include <string>
#include <unordered_map>

namespace msg5::config {

    // Кто мы: CLI / FILE / ENV / STDIN (добавляйте по мере надобности)
    enum class ProviderClass : unsigned char { Cli, File, Env, Stdin };

    inline constexpr const char* to_string(ProviderClass k) {
        switch (k) {
            case ProviderClass::Cli:   return "cli";
            case ProviderClass::File:  return "file";
            case ProviderClass::Env:   return "env";
            case ProviderClass::Stdin: return "stdin";
        }
        return "unknown";
    }
    // Результат работы источника: сырые пары "канонический ключ -> строковое значение".
    // Оставляем struct — можно безболезненно расширять (счётчики, тайминги и т.п.).
   
    // Категория происхождения значения (для отчёта пользователя)
    enum class ValueSource { Default, Cli, ConfigFile, Env, Stdin };
    
    inline constexpr const char* to_string(ValueSource o) {
        switch (o) {
        case ValueSource::Default:    return "default";
        case ValueSource::Cli:        return "cli";
        case ValueSource::ConfigFile: return "config";
        case ValueSource::Env:        return "env";
        case ValueSource::Stdin:      return "stdin";
        }
        return "unknown";
    }
        
    // Единая точка маппинга механизма адаптера в категорию происхождения
    static constexpr ValueSource value_source_of(ProviderClass k) noexcept {
        switch (k) {
        case ProviderClass::Cli:   return ValueSource::Cli;
        case ProviderClass::File:  return ValueSource::ConfigFile;
        case ProviderClass::Env:   return ValueSource::Env;
        case ProviderClass::Stdin: return ValueSource::Stdin;
        }
        return ValueSource::Default;
    }
        
    // Результат fetch: пары + источник происхождения
    struct FetchResult {
        std::unordered_map<std::string, std::string> kv;
        ValueSource source{ ValueSource::Default };
    };

    // Вперёд-объявление спецификации (детали не тянем в этот хедер).
    struct CommandSpec;

} // namespace msg5::config
