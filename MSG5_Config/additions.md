# Конвенции параметров — кратко (нормализация регистра)

## Поток (без изменений)
1) Источники читают значения (приоритет: CLI > FILE > ENV > STDIN).  
2) `Resolver` сливает по приоритету в `ResolvedOptions::push(..., override=false)`.  
3) `apply_defaults()` — заполняет пропуски (пока заглушка).  
4) `validate_against_spec()` — required/type/enum.  

## Имена ключей и регистр
- **Канон в спецификации**: ключи описываем в **lower.case** с точками, напр. `db.host`, `log.level`.
- **CLI / FILE / STDIN**: при чтении **внутренне приводим ключи к lower.case**  
  (т.е. `--DB.HOST`, `"DB.HOST"` → `db.host`). Это снимает зависимость от CapsLock.
- **ENV**: ищем только `MSG5_` + **UPPER_SNAKE** от канона  
  (`db.host` → `MSG5_DB_HOST`). В `ResolvedOptions` ключ хранится в **lower.case** (`db.host`).

## Неизвестные / недостающие
- **Unknown key** (любой источник): не падаем → **user-warning** и игнор.  
- **Missing key**:  
  - есть дефолт → подставим (origin=Default);  
  - `required` без дефолта → **user-error**;  
  - необязательный → пропускаем.

## Ошибки значений
- Неверный тип / невалидный enum → **user-error** (останавливаемся).
- Системные проблемы источника (I/O, парсинг) → **system-error** или «тихо пусто» по политике конкретного источника.

## Пример нормализации
- CLI: `--DB.HOST=localhost` → ключ `db.host`, origin=CLI.  
- FILE/STDIN: `"DB.PORT": 5432` → ключ `db.port`, origin=FILE/STDIN.  
- ENV: `MSG5_DB_HOST=localhost` → ключ `db.host`, origin=ENV.

## Диагностика
- Все предупреждения/ошибки — человекочитаемые, с указанием ключа и источника.  
- Печать эффективной конфигурации (позже): маскируем секреты, показываем origin.
