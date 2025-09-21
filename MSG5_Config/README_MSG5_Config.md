# MSG5_Config — каркас конфиг-движка (VS2022, C++)

> Мини-док для разработчика. Коротко про структуру, сборку, приоритеты источников и ближайший план работ (P1–P8).

## TL;DR
- **Собирается** в Visual Studio 2022 (x64, Release/Debug).
- Публичные заголовки: `include/msg5/config/*`.
- Источники конфигурации (CLI/FILE/ENV/STDIN) реализованы через `SourceBase` в `src/providers/`.
- Резолвинг: `Resolver` → слияние источников по приоритету → `apply_defaults()` → `validate()`.
- Дальше по плану: печать эффективной конфигурации, `--help`, `--dry-run`, тесты.

---

## Структура каталогов
```
MSG5_Config/
├─ include/
│  └─ msg5/
│     └─ config/
│        ├─ CommandSpec.h        // описание опций (OptionSpec) и типов (OptionType)
│        ├─ IOptionsSource.h     // интерфейс источника (fetch + kind)
│        ├─ ResolvedOptions.h    // итоговые значения + origin
│        ├─ Resolver.h           // фасад-оркестратор
│        └─ errors.h             // user_error/system_error + ExitCode
├─ src/
│  ├─ core/
│  │  ├─ ConfigResolver.cpp      // legacy-логика (постепенно худеет)
│  │  ├─ MergeEngine.cpp         // слияние (если отдельно от Resolver)
│  │  ├─ Resolver.cpp            // текущий оркестратор (использует Sources)
│  │  ├─ DefaultsFiller.cpp      // подстановка дефолтов (P4)
│  │  └─ Validators.cpp          // валидация по CommandSpec (P4)
│  ├─ providers/
│  │  ├─ SourceBase.h            // базовый класс с kind()
│  │  ├─ ArgsSource.h/.cpp       // разбор argv (--k=v, --k v, -abc)
│  │  ├─ FileSource.h/.cpp       // чтение config.json (плоский JSON)
│  │  ├─ EnvSource.h/.cpp        // окружение (ENV), opt.env_names и нормализация ключа
│  │  └─ PromptSource.h/.cpp     // stdin (плоский JSON из pipe)
│  └─ util/
│     ├─ cli_parse.h/.cpp        // простой парсер CLI флагов
│     ├─ json_utils.h/.cpp       // плоский JSON → map<string,string>
│     └─ env_utils.h/.cpp        // (по мере необходимости)
├─ MSG5_Config.vcxproj
├─ MSG5_Config.vcxproj.filters
└─ pch.h / pch.cpp               // precompiled header
```

## Сборка (Visual Studio 2022)
1. Открой `MSG5_Config.vcxproj`.
2. Проверь свойства проекта:
   - **C/C++ → Additional Include Directories**: `$(ProjectDir)include`
   - **C/C++ → Language**: C++17+ (рекомендуется)
   - **Precompiled Headers**: все `.cpp` начинают с `#include "pch.h"`
3. Собери **x64 / Release** или **Debug**.

## Использование (в коде)
```cpp
#include "msg5/config/Resolver.h"
#include "src/providers/SourceBase.h"
#include "src/providers/ArgsSource.h"
#include "src/providers/FileSource.h"
#include "src/providers/EnvSource.h"
#include "src/providers/PromptSource.h"
using namespace msg5::config;

CommandSpec spec;
// заполнить spec.options: vector<OptionSpec>{ ... }

Resolver r({
  std::make_unique<ArgsSource>(argc, argv),
  std::make_unique<FileSource>();
  std::make_unique<EnvSource>();
  std::make_unique<PromptSource>();
});
auto ro = r.resolve(spec);
// ro.values / ro.origins — итог
```

## Приоритет источников
Приоритет задаётся порядком вектора при создании `Resolver` (ранее = сильнее):  
`CLI > FILE > ENV > STDIN` — рекомендованный дефолт.  
Можно использовать другой порядок под задачу.

## Defaults & Validation (P4)
- `apply_defaults(const CommandSpec&, ResolvedOptions&)` — подставляет `default_value` в пустые ключи, помечая `Origin::Default`.
- `validate_against_spec(const CommandSpec&, const ResolvedOptions&)` (или `ResolvedOptions::validate`) — проверяет:
  - `required` (обязательные),
  - `type` (`Int`, `Bool`, `String`, `Path`, `Enum`),
  - `enum_values` (для `Enum`).

## Печать «эффективной» конфигурации (P5, план)
- Функция `dump_effective(spec, ro, show_secrets=false)`:
  - Маскирует `OptSecret` (`•••••`), если `show_secrets=false`.
  - Показывает источник: `(CLI|FILE|ENV|DEFAULT)`.

## Флаги эксплуатации (P6, план)
- `--help` — генерация подсказки по `CommandSpec`.
- `--dry-run` — показать итоговую конфигурацию без побочных действий.
- Единые коды выхода: `EXIT_OK/EXIT_USER_ERR/EXIT_SYS_ERR/EXIT_UNKNOWN_ERR`.

## Roadmap (статус)
- ✅ **P1**: единый интерфейс источников (`SourceBase`, `*Source`).
- ✅ **P2**: `ArgsSource` + `cli_parse` (минимум).
- ✅ **P3**: `FileSource` (плоский JSON), `EnvSource`, `PromptSource`.
- ⏳ **P4**: `DefaultsFiller` + `Validators` — каркас добавлен, заполнить валидацию под твои `OptionType`.
- ⏳ **P5**: `dump_effective()` + маскирование секретов.
- ⏳ **P6**: `--help`, `--dry-run`, коды выхода.
- ⏳ **P7**: ENV-префиксы/нормализация/alias (по задаче).
- ⏳ **P8**: GTest-проекты (два .exe), smoke-тесты источников и приоритета.

## Правила код-стайла для читабельного main (рекомендации)
- `main` → `app.init(); app.run(); app.done();`
- Внутри `init()` — чек-лист: `prepare_parameters()`, `prepare_logging()`, `prepare_connections()`, `prepare_environment()`.
- В `run()` — нумерованные шаги, лог `[i/N]`, поддержка `--dry-run`.
- В `done()` — стабильная очистка/закрытие ресурсов.

## Частые проблемы и решения
- **C1083: ResolvedOptions.h not found** — проверь `Additional Include Directories = $(ProjectDir)include` и инклюды вида `#include "msg5/config/ResolvedOptions.h"`.
- **C4996: getenv** — используй безопасный враппер на `_dupenv_s` (см. `EnvSource.cpp` → `safe_getenv`).
- **PCH ошибки** — в каждом `.cpp` первой строкой `#include "pch.h"`.
- **Namespace рассинхрон** — форвард-дефы (`struct CommandSpec;`) объявляй **внутри** `namespace msg5::config` и в `.h` подключай реальный заголовок, если тип используется в сигнатуре.

---

## Лицензия
MIT (или согласованная в основном репозитории).
