# MSG5_Config — каркас конфиг-движка (VS2022, C++20)

> Коротко про структуру, сборку, приоритеты источников и план (P1–P8).

## TL;DR
- Сборка: **Visual Studio 2022**, x64, Debug/Release.
- Публичные заголовки: `include/msg5/config/**` (в т.ч. `include/msg5/config/source/*`).
- Источники конфигурации (CLI / FILE / ENV / STDIN) реализованы через `SourceBase`; реализации лежат в `src/providers/`.
- Резолвинг: `Resolver` → слияние источников по приоритету → *(план)* `apply_defaults()` → *(план)* `validate()`.

---

## Структура каталогов

```
MSG5_Config/
├─ include/
│  └─ msg5/config/
│     ├─ ResolvedOptions.h          // итоговые значения + origin
│     ├─ Resolver.h                 // фасад-оркестратор
│     ├─ OptionsSourceTypes.h       // SourceKind, FetchResult и пр.
│     ├─ CommandSpec.h              // спецификация опций/флагов
│     └─ source/
│        ├─ IOptionsSource.h        // тонкий контракт источников
│        └─ SourceBase.h            // базовый класс (fetch no-throw, id(), kind(), emit)
├─ src/
│  ├─ core/
│  │  ├─ Resolver.cpp               // оркестратор (prepare → fetch → merge «ранний сильнее»)
│  │  ├─ DefaultsFiller.cpp         // (P4, план) подстановка дефолтов
│  │  └─ Validators.cpp             // (P4, план) валидация по CommandSpec
│  ├─ providers/
│  │  ├─ ArgsSource.h/.cpp          // CLI: разбор argv (--k=v, --k v, -abc)
│  │  ├─ FileSource.h/.cpp          // FILE: один путь → плоский JSON → пары по spec
│  │  ├─ EnvSource.h/.cpp           // ENV: env_names → PREFIX+TO_ENV_KEY(key)
│  │  └─ PromptSource.h/.cpp        // STDIN: плоский JSON из pipe → по spec
│  └─ util/
│     ├─ cli_parse.h/.cpp           // простой парсер CLI-флагов
│     ├─ json_utils.h/.cpp          // плоский JSON → map<string,string>
│     └─ env_utils.h/.cpp           // безопасный getenv, нормализация имён
├─ MSG5_Config.vcxproj
├─ MSG5_Config.vcxproj.filters
└─ pch.h / pch.cpp                  // precompiled header
```

---

## Сборка (Visual Studio 2022)

- Конфигурация: `x64 / Debug` или `x64 / Release`.
- Флаги языка: `/std:c++20`, `/permissive-`, `/utf-8`.
- Каждый `.cpp` начинается с `#include "pch.h"`.

Сборка из IDE:
```
Solution Explorer → правый клик на MSG5_Config → Build
```

Сборка из CLI:
```
msbuild MSG5_Config.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
```

---

## Приоритет источников

**Порядок задаёт вызывающий код**: вы передаёте источники в `Resolver` в нужной последовательности.  
Действует правило *«ранний сильнее»*: если ключ уже установлен более ранним слоем, поздние его **не перезаписывают**.

Рекомендуемый дефолт порядка слоёв:
```
CLI  >  FILE  >  ENV  >  STDIN
```
(Можно использовать и другой порядок под задачу.)

---

## Поведение источников (контракт)

Общее:
- Источники **никогда не бросают исключения наружу**; все проблемы → события `emit(...)`.
- Каждое событие содержит `source_id` (например, `cli`, `file:<path>`, `env:<prefix>`, `stdin`).
- Источники возвращают **только** пары, соответствующие ключам из `CommandSpec`:
  - Для FILE/STDIN: ищем по `json_path`, иначе по `key`.
  - Для ENV: сначала `opt.env_names` (если заданы), иначе `PREFIX + TO_ENV_KEY(key)`.
  - Для CLI: только заявленные `cli_flags`; неизвестные флаги игнорируются и логируются.

Конкретика:
- **ArgsSource (CLI)**: в `prepare(spec)` строит `flag → key`, в `fetch_impl` возвращает только разрешённые флаги; неизвестные → `INFO CLI_FLAG_UNKNOWN` (можно поднять до WARN при желании).
- **FileSource (FILE)**: «тупой» — один путь → плоский JSON → пары только по ключам/пути из `spec`. Ошибки: `FILE_NOT_FOUND`, `FILE_EMPTY`, `FILE_READ_OK`.
- **EnvSource (ENV)**: префикс приложения обязателен (например, `MSG5_`). Порядок поиска: `env_names` → `PREFIX + TO_ENV_KEY(key)`. Секреты маскируются в логах.
- **PromptSource (STDIN)**: читает до EOF; парсит плоский JSON; возвращает только ключи из `spec`; события `STDIN_*`.

---

## Defaults & Validation (P4, план)

После слияния слоёв:
- `apply_defaults(const CommandSpec&, ResolvedOptions&)` — подставляет `default_value` в пустые ключи; origin помечается `Origin::Default`.
- `validate_against_spec(const CommandSpec&, const ResolvedOptions&)` — проверяет:
  - `required` (обязательность),
  - `type` (`Int`, `Bool`, `String`, `Path`, `Enum`),
  - `enum_values` (для `Enum`).

Ошибки валидации классифицируются как **user-error**, системные ошибки (I/O, parse) — как **system-error**.

---

## Печать «эффективной» конфигурации (план)

- `dump_effective(options, origins)` — вывод ключей, значений и их источника (`Cli/File/Env/Stdin/Default`), маскируя секреты.
- `--dry-run` — показать итог без побочных действий.
- Коды выхода: `EXIT_OK / EXIT_USER_ERR / EXIT_SYS_ERR / EXIT_UNKNOWN_ERR`.

---

## Пример использования (минимум)

> В примере заголовки провайдеров подключены из текущего проекта.  
> Если нужно скрыть реализацию, добавьте фабрики `createArgsSource(...)`, `createFileSource(...)` и т.п. в публичный API.

```cpp
#include "msg5/config/Resolver.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/source/IOptionsSource.h"
#include "msg5/config/source/SourceBase.h"

// провайдеры (расположение может отличаться в вашем проекте):
#include "src/providers/ArgsSource.h"
#include "src/providers/FileSource.h"
#include "src/providers/EnvSource.h"
#include "src/providers/PromptSource.h"

using namespace msg5::config;

int main(int argc, const char* argv[]) {
  CommandSpec spec = /* ... заполняем список options ... */;

  std::vector<IOptionsSourcePtr> sources;
  sources.emplace_back(std::make_unique<ArgsSource>(argc, argv));
  sources.emplace_back(std::make_unique<FileSource>("config/user.json"));
  sources.emplace_back(std::make_unique<EnvSource>("MSG5_"));
  sources.emplace_back(std::make_unique<PromptSource>());

  Resolver r(std::move(sources));
  const auto resolved = r.resolve(spec);

  // resolved.values  — итоговые значения (string)
  // resolved.origins — источник значения (Origin::Cli/File/Env/Stdin/Default)
  return 0;
}
```

---

## Roadmap (статус)

- ✅ **P1**: Единый интерфейс источников (`SourceBase`, `*Source`).
- ✅ **P2**: `ArgsSource` + `cli_parse` (минимум).
- ✅ **P3**: `FileSource` (плоский JSON), `EnvSource`, `PromptSource`.
- ⏳ **P4**: `DefaultsFiller` + `Validators` — подготовить типы и проверки.
- ⏳ **P5**: `dump_effective()` + маскирование секретов.
- ⏳ **P6**: `--help` / `--dry-run` / коды выхода.
- ⏳ **P7**: Тесты провайдеров и интеграционные тесты резолвера.
- ⏳ **P8**: Поддержка `json_path` с точечной нотацией (вложенный JSON) или переход на `nlohmann::json` в `.cpp`.

---

## Частые проблемы и решения

- **C1083: `<Header>.h` not found** — проверь пути инклюдов: для публичных хедеров всегда `#include "msg5/config/..."`.
- **PCH mismatch** — у каждого `.cpp` первая строка должна быть `#include "pch.h"`.
- **“does not override”** — провайдеры должны переопределять **`fetch_impl(const CommandSpec&)`**, а не `fetch(...)` (он уже no-throw в базе).
- **Unresolved externals** — проверь, что `.cpp` провайдера/утилит добавлены в проект или тянуты зависимостью (Project Reference).
- **`getenv`/безопасность** — используйте `env_utils` (`_dupenv_s` на Windows) вместо прямого `getenv`.

---

## Лицензия

MIT (или согласованная в основном репозитории).

