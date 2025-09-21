App/Main
  │
  │ 1. Готовим CommandSpec (options: vector<OptionSpec>)
  │    и источники (в порядке приоритета)
  ▼
Resolver
  │
  │ for src in sources:               // пример: Args > File > Env > Prompt
  │   r = src.fetch(spec)             // FetchResult { kv: map<string,string> }
  │   for (k,v) in r.kv:
  │     if k not in ro.values:        // ранний источник — сильнее
  │        ro.values[k]  = v
  │        ro.origins[k] = origin(src.kind())
  │
  │ ro.check_invariants()             // контейнерные правила (внутренние)
  │ apply_defaults(spec, ro)          // сейчас NO-OP (заглушка)
  │ validate_against_spec(spec, ro)   // правила из спецификации
  │
  ▼
ResolvedOptions  ──► потребители (инициализация компонентов, подключение к БД, и т.п.)
