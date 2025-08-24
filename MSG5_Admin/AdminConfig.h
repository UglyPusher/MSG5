#pragma once
#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include <string>
#include <nlohmann/json.hpp>
#include "Config/BasicConfig.h"

namespace msg5::config {

    struct AdminConfig : BasicConfig {
        // Пути
        std::string baseline_dir;   // путь к baseline
        std::string migrations_dir; // путь к каталогу миграций

        // DSN-ы
        std::string bootstrap_dsn;  // повышенные права (создание БД/ролей и т.п.)
        std::string app_dsn;        // «боевой» DSN владельца схемы/приложения

        // JSON → поля (читаем либо из корня admin.json, либо из секции "admin" — обе схемы допустимы)
        void from_json(const nlohmann::json& j);

        // ENV overrides: MSG5_SERVER_PORT (из BasicConfig) + свои:
        // MSG5_BASELINE_DIR, MSG5_MIGRATIONS_DIR, MSG5_BOOTSTRAP_DSN, MSG5_APP_DSN
        void apply_env_overrides(const char* prefix = "MSG5_");

        // Базовая валидация путей (DSN проверим уже в конкретных ручках по месту)
        void validate() const;
    };

} // namespace msg5::config
