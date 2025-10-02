#pragma once
//
// MSG5_Config — интерфейс источника опций (адаптера).
// «Тонкий» контракт поверх базовых типов из OptionsSourceTypes.h.
// Общая логика (логирование, try/catch-обёртка и т.п.) — вне этого файла.
//

#include <memory>              // std::unique_ptr
#include <string>              // std::string (для id())
#include "msg5/config/OptionsSourceTypes.h"

namespace msg5::config {

    /// Базовый интерфейс источника опций.
    struct IOptionsSource {
        virtual ~IOptionsSource() = default;

        // Тип источника (для origin/отчётов).
        virtual ProviderClass kind() const = 0;

        // Человекочитаемый идентификатор (напр. "cli", "file:D:\\cfg\\msg5.json", "env:MSG5_", "stdin").
        virtual std::string id() const = 0;

        // Опциональная подготовка под конкретную спецификацию (кэш маппингов и т.п.).
        // Резолвер вызывает один раз ДО первого fetch(spec). Исключения бросать не рекомендуется.
        virtual void prepare(const CommandSpec& /*spec*/) {}

        // Главный вызов: получить пары key->string для ключей из spec.
        // ВАЖНО: noexcept — любые ошибки источник конвертирует во внутренние события/логи
        // и возвращает частичный/пустой результат. «Всегда продолжаем».
        virtual FetchResult fetch(const CommandSpec& spec) noexcept = 0;

        // Запрещаем копирование/перемещение через базовый интерфейс.
        IOptionsSource() = default;
        IOptionsSource(const IOptionsSource&) = delete;
        IOptionsSource& operator=(const IOptionsSource&) = delete;
        IOptionsSource(IOptionsSource&&) = delete;
        IOptionsSource& operator=(IOptionsSource&&) = delete;
    };

    // Удобный алиас владения источником
    using IOptionsSourcePtr = std::unique_ptr<IOptionsSource>;

} // namespace msg5::config
