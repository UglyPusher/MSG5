# MSG5_Config — Руководство разработчика

Это практическая документация по подключению и использованию конфиг-движка **MSG5_Config** в проектах на C++ (VS2022, C++20), а также по вспомогательному CLI‑тулу `MSG5_Config_Tool`.

## Содержание
1. Структура репозитория и ключевые файлы
2. Сборка (VS2022 / MSBuild)
3. Подключение библиотеки в приложение
4. Источники конфигурации и приоритеты
5. Пайплайн резолвера (collect → defaults → validate)
6. Дефолты и валидация (интерфейсы и политика)
7. CLI‑тул: генерация шаблона и дамп «эффективной» конфигурации
8. Конвенции и чек‑лист
9. Траблшутинг

---

## 1) Структура репозитория и ключевые файлы

```
MSG5_Config/
├─ include/msg5/config/
│  ├─ ResolvedOptions.h          # итоговые значения + источник (origin)
│  ├─ Resolver.h                 # фасад-оркестратор
│  ├─ OptionsSourceTypes.h       # типы: SourceKind, ValueSource и пр.
│  ├─ CommandSpec.h              # спецификация опций/флагов
│  ├─ Sources.h                  # публичные фабрики провайдеров
│  └─ source/
│     ├─ IOptionsSource.h        # контракт источников
│     └─ SourceBase.h            # базовая реализация (emit, id, min_level)
├─ src/core/
│  ├─ Resolver.cpp               # prepare→fetch→merge→defaults→validate
│  ├─ Sources.cpp                # фабрики источников
│  ├─ DefaultsFiller.cpp         # (опц.) подстановка дефолтов
│  └─ Validators.cpp             # (опц.) валидация
├─ src/providers/
│  ├─ ArgsSource.h/.cpp          # CLI (--k=v, --k v, -abc)
│  ├─ FileSource.h/.cpp          # FILE (плоский JSON)
│  ├─ EnvSource.h/.cpp           # ENV (префикс + env_names)
│  └─ PromptSource.h/.cpp        # STDIN (плоский JSON из pipe)
└─ src/util/…                    # утилиты (cli_parse, env_utils, json_utils)
```

> Публичные хедеры берём из `include/msg5/config/**`.

---

## 2) Сборка (VS2022 / MSBuild)

- Конфигурация: `x64 / Debug` или `x64 / Release`
- Язык: `/std:c++20`, `/permissive-`, `/utf-8`
- PCH: каждый `.cpp` начинается с `#include "pch.h"`

Сборка из IDE: **Build → Build MSG5_Config**  
Сборка из CLI:
```bat
msbuild MSG5_Config.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
```

Результат: `x64/Release/MSG5_Config.lib`

---

## 3) Подключение библиотеки в приложение

В вашем `.vcxproj`:
- **Include Directories**: укажите путь на `MSG5_Config/include` и (при необходимости) `third_party/nlohmann`
- **Additional Dependencies**: линкуйте `MSG5_Config.lib`

Минимальный пример использования:

```cpp
#include "msg5/config/Resolver.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/Sources.h"

using namespace msg5::config;

int main(int argc, const char* argv[]) {
  CommandSpec spec = /* заполнить список OptionSpec */;

  std::vector<IOptionsSourcePtr> sources;
  sources.emplace_back(makeArgsSource(argc, argv));
  sources.emplace_back(makeFileSource("config/user.json"));
  sources.emplace_back(makeEnvSource("MSG5_"));
  sources.emplace_back(makePromptSource(/*interactive=*/true));

  Resolver resolver(std::move(sources));
  resolver.set_min_level(LogLevel::Info);
  resolver.subscribe([](const LogEvent& ev){
    // мост логов в вашу систему логирования
  });

  ResolveParams params{};
  params.apply_defaults = true;
  params.validate       = true;
  ResolvedOptions::ValidationResult vr;
  params.out_errors = &vr;

  ResolvedOptions ro = resolver.resolve(spec, params);

  if (!vr.empty()) {
    // обработать ошибки валидации
  }

  // использовать ro.values[...] по ключам из spec
  return 0;
}
```

---

## 4) Источники конфигурации и приоритеты

Приоритет задаётся **порядком вектора** источников, который вы передаёте в `Resolver`. Действует правило **«ранний сильнее»**: значение, однажды установленное ранним источником, поздними не перезаписывается.

Рекомендуемый порядок по умолчанию:
```
CLI  >  FILE  >  ENV  >  PROMPT(STDIN)
```

Поведение источников:
- **ArgsSource (CLI)** — принимает только флаги, объявленные в `OptionSpec.cli_flags`. Неизвестные флаги игнорируются и логируются как `CLI_FLAG_UNKNOWN`.
- **FileSource (FILE)** — читает **плоский JSON** (`{ "k": "v" }`), для каждого ключа из `spec` ищет сначала `json_path`, затем `key`. Лишние поля игнорируются.
- **EnvSource (ENV)** — ищет либо по `opt.env_names`, либо по `PREFIX + TO_ENV_KEY(opt.key)` (`db.host` → `DB_HOST`), префикс, напр., `MSG5_`. Пустая строка допустима.
- **PromptSource (STDIN)** — парсит плоский JSON из stdin. Удобно для неинтерактивных пайпов, можно отключить через флаг приложения.

Секреты (флаг `OptSecret`) в логах **маскируются**, но в `ResolvedOptions` хранятся как есть.

---

## 5) Пайплайн резолвера

Последовательность шагов `Resolver::resolve`:
1. `prepare(spec)` для каждого источника (кэш сопоставлений флагов/путей).
2. Сбор пар `(key, value)` от источников в заданном порядке (**ранний сильнее**).
3. `apply_defaults(spec, ro)` — если включено в `ResolveParams`.
4. `validate(spec, ro, out_errors)` — если включено.

Все источники и резолвер **не бросают исключений наружу**; проблемы и диагностика идут через `emit(...)` (уровни `Info/Warn/Error`).

---

## 6) Дефолты и валидация

- **Дефолты**: из `OptionSpec.default_value` → проставляются в пустые ключи; источник — `ValueSource::Default`.
- **Валидация** (без исключений): возвращает `bool` и заполняет список ошибок:
  - `required` — значение обязательно;
  - `type` — `Bool/Int/String/Enum/Path` (минимальный набор);
  - `enum` — допускаются только значения из `enum_values`.

Политика: если приложению валидация **не нужна** — просто выключите её в `ResolveParams` и не вызывайте отдельно.

---

## 7) CLI‑тул `MSG5_Config_Tool`

Упрощает работу с file‑mode и отладку:

```
MSG5_Config_Tool.exe [опции-конфига] [опции-тула]
```

Опции конфига (зависят от вашей `CommandSpec`):
```
--db.host=HOST
--db.user=USER
--db.password=PASS
--log.level=LEVEL     trace|debug|info|warn|error
```

Опции тула:
```
--gen.template=PATH   Сгенерировать шаблон (плоский JSON, dotted-ключи)
--dump.effective=PATH Дамп «эффективной» конфигурации после resolve
--config=PATH         Альтернативный файл вместо config/user.json
--no-prompt           Отключить PromptSource (не интерактив)
--help | -h | /?      Справка
```

Примеры:
```bat
:: 1) Сгенерировать шаблон
MSG5_Config_Tool.exe --gen.template=config/user.template.json

:: 2) Резолв + дамп результата без интерактива
MSG5_Config_Tool.exe --db.host=cli --db.user=cli --no-prompt ^
  --dump.effective=config/user.effective.json
```

Формат файла — **плоский JSON** с dotted‑ключами, совместимый с `FileSource`.

---

## 8) Конвенции и чек‑лист

- Канонические имена — `lower.dotted.case` (`db.host`, `log.level`).
- Источники возвращают **только** ключи из `CommandSpec`.
- Никаких исключений наружу; диагностика через `emit(...)`.
- Секреты маскируются в логах, в результат кладётся фактическое значение.
- Порядок слоёв задаётся вызывающим кодом; «ранний сильнее».
- FILE/STDIN сейчас — **плоский JSON** (без «магии» вложенности).
- CLI — только заявленные `cli_flags`; неизвестные логируются.
- ENV — `env_names` или `PREFIX + TO_ENV_KEY(key)`.

Быстрый чек‑лист перед коммитом:
- [ ] Сборка `Release x64` зелёная
- [ ] Прогон тула: `--help`, `--gen.template`, `--dump.effective`
- [ ] Набор логов стабилен (INFO/WARN/ERROR коды)
- [ ] Тесты провайдеров/интеграция резолвера — зелёные

---

## 9) Траблшутинг

- **Гарбаж вместо кириллицы в `--help` на Windows**: собирать с `/utf-8` и в `main()` вызвать `SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);`; либо `chcp 65001` в `cmd`/использовать PowerShell.
- **PCH mismatch**: каждый `.cpp` должен начинаться с `#include "pch.h"`.
- **Unresolved externals**: не забыть добавить `.cpp` реализаций (провайдеры/утилиты) в проект.
- **Не читается значение из ENV**: проверь префикс `MSG5_` и имя `TO_ENV_KEY(key)` (`db.host` → `DB_HOST`). Пустая строка допустима и не равна «переменной нет».
- **FileSource игнорирует поле**: ключ отсутствует в `CommandSpec` или неправильное имя (`json_path`/`key`).

---

© MSG5 — используйте по политике проекта.
