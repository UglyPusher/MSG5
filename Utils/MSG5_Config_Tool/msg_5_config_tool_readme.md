# MSG5_Config_Tool

CLI-утилита для:
- генерации шаблона конфигурации для **file-mode**,
- дампа «эффективной» конфигурации после резолва (CLI > FILE > ENV > PROMPT),
- опционального применения **дефолтов** и **валидации** по `CommandSpec`.

> Библиотека `MSG5_Config` остаётся тонкой (без I/O и JSON). Тул — отдельный проект.

---

## Сборка

1. Соберите библиотеку:
   - **Solution Configuration**: `Release`
   - **Platform**: `x64`
   - Проект: `MSG5_Config` → получает `x64/Release/MSG5_Config.lib`

2. Соберите утилиту:
   - Проект: `MSG5_Config_Tool`
   - Зависимости:
     - Include: `MSG5_Config/include`, `third_party/nlohmann`
     - Link: `x64/Release/MSG5_Config.lib`
   - Рекомендуемые флаги:
     - C++20, `/permissive-`, `/utf-8`

---

## Запуск

```bat
x64\Release\MSG5_Config_Tool.exe [опции-конфига] [опции-тула]
```

### Опции конфига (пример; зависят от вашей `CommandSpec`)
```
--db.host=HOST
--db.user=USER
--db.password=PASS      (секрет; в дампе маскируется)
--log.level=LEVEL       trace|debug|info|warn|error
```

### Опции тула
```
--gen.template=PATH     Сгенерировать шаблон (плоский JSON с dotted-ключами)
--dump.effective=PATH   Сохранить «эффективную» конфигурацию после resolve
--no-prompt             Отключить интерактивные вопросы (PromptSource)
--help | -h | /?        Справка и выход
--version               Печать версии и выход (если включено)
```

---

## Примеры

### 1) Генерация шаблона file-mode
```bat
x64\Release\MSG5_Config_Tool.exe ^
  --gen.template=config/user.template.json
```

### 2) Резолв (CLI+FILE+ENV), дефолты, валидация + дамп результата
```bat
x64\Release\MSG5_Config_Tool.exe ^
  --db.host=cli.local --db.user=cli --no-prompt ^
  --dump.effective=config/user.effective.json
```

> `--no-prompt` отключает интерактив `PromptSource` (иначе тул спросит отсутствующие поля).

---

## Приоритет источников

1. **CLI** (`--key=value`)
2. **FILE** (`config/user.json` — плоский JSON с dotted-ключами)
3. **ENV** (с префиксом `MSG5_`, например `MSG5_DB_PASSWORD`)
4. **PROMPT** (интерактив, если не выключен `--no-prompt`)

Ранние источники **сильнее**: найденное ранее значение **не перезаписывается**.

---

## Форматы файлов

### Шаблон и «эффективный» дамп (плоский JSON)

```json
{
  "db.host": "cli.local",
  "db.user": "cli",
  "db.password": "pa********",
  "log.level": "info",
  "__source": {
    "db.host": "cli",
    "db.user": "cli",
    "db.password": "default",
    "log.level": "config"
  }
}
```

- Ключи — **dotted** (`db.host`, `log.level`) — совместимо с `FileSource`.
- Секреты (флаг `OptSecret`) **маскируются** в дампе по умолчанию.
- Поле `__source` добавляется при дампе (можно отключить в коде).

---

## Дефолты и валидация

- Дефолты из `OptionSpec.default_value` применяются **после сбора** и **до валидации**.
- Валидация возвращает `bool` и список ошибок:
  - `required` — отсутствие значения,
  - `enum` — недопустимое значение,
  - `int` — некорректный формат/диапазон (если такие правила заданы).
- Логи: `DEFAULTS_APPLIED`, `VALIDATION_OK` / `VALIDATION_ERROR`, `VALIDATION_SUMMARY`.

---

## Коды возврата

- `0` — успешно (валидация прошла).
- `3` — валидация не прошла (есть ошибки).
- `2` — ошибка записи шаблона.
- `4` — ошибка записи дампа.
- `1` — прочие исключения (фатальные ошибки).

---

## Логирование

- Единый формат: `[log] <LVL> src=<source> code=<code> msg=<message>`
- Уровень можно менять в коде (`resolver.set_min_level(LogLevel::Trace)`).
- Логи источников транслируются подписчикам тула.

---

## Советы (Windows/кодировка)

- Консоль Windows: используйте PowerShell/Windows Terminal или
  выполните `chcp 65001` перед запуском.
- Проект собирать с `/utf-8`. В коде установлены `SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);`.

---

## Встраивание в ваш проект

- `CommandSpec` определяет набор опций (ключ, тип, enum-значения, `default_value`, `required`).
- Подключите собственную фабрику `make_spec()` вместо примера в туле.
- Формат `config/user.json` — плоский JSON (dotted-ключи) для простого file-mode.

---

## Лицензия

В составе репозитория MSG5. Используйте согласно политике проекта.

