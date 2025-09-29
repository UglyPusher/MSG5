#pragma once
//
// MSG5_Config — базовые типы источников (не тянут интерфейс).
// Подходит для инклуда из основной аппки.
//

#include <string>
#include <unordered_map>

namespace msg5::config {

    // Кто мы: CLI / FILE / ENV / STDIN (добавляйте по мере надобности)
    enum class SourceKind : unsigned char { Cli, File, Env, Stdin };

    // Результат работы источника: сырые пары "канонический ключ -> строковое значение".
    // Оставляем struct — можно безболезненно расширять (счётчики, тайминги и т.п.).
    struct FetchResult {
        std::unordered_map<std::string, std::string> kv;
    };

    // Вперёд-объявление спецификации (детали не тянем в этот хедер).
    struct CommandSpec;

} // namespace msg5::config
