# Базовая логика пайплайна (MSG5_Config)

```text
App/Main
  │
  │ 1) Готовим CommandSpec (options: vector<OptionSpec>)
  │    и источники (в ТРЕБУЕМОМ порядке приоритета)
  ▼
Resolver
  │
  │ 0) prepare(spec) для КАЖДОГО источника (кэш флагов/карт и т.п.)
  │
  │ 1) for src in sources:                // пример: Args > File > Env > Prompt
  │      r = src.fetch(spec)              // no-throw; FetchResult { kv: map<string,string> }
  │      for (k, v) in r.kv:
  │        if k not in ro.values:         // «ранний сильнее» — НЕ перезаписываем установленное
  │          ro.values[k]  = v
  │          ro.origins[k] = origin(src.kind())
  │
  │ 2) (опц.) собрать события источников для отчёта/телеметрии
  │    — FILE_* / ENV_* / CLI_* / STDIN_*; секреты маскируются
  │
  │ 3) apply_defaults(spec, ro)           // заглушка (P4): проставить default_value с Origin::Default
  │
  │ 4) validate_against_spec(spec, ro)    // заглушка (P4): required/type/enum
  │
  ▼
ResolvedOptions  ──► потребители (инициализация компонентов, подключение к БД, и т.п.)
```

## Примечания
- Порядок слоёв задаёт вызывающий код. Рекомендация по умолчанию: `CLI > FILE > ENV > STDIN`.
- Маппинг `SourceKind → Origin`: `Cli`, `ConfigFile`, `Env`, `Stdin`.
- Источники не бросают исключений; любые проблемы — в событиях (`emit`).
- FILE/STDIN/CLI/ENV возвращают только ключи, заданные в `CommandSpec`. Секреты маскируются в логах.

