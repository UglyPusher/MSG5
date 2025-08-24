#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Config/BasicConfig.h"

namespace msg5::config {

    struct ClientConfig : BasicConfig {
        std::string pg_dsn; // может быть пустым (тогда /api/db/version вернёт 500)

        // заполнение из client/user.json
        void from_json(const nlohmann::json& j) {
            BasicConfig::from_json(j); // server_port (если есть)
            if (j.contains("pg_dsn")) {
                j.at("pg_dsn").get_to(pg_dsn);
            }
            else if (j.contains("dsn")) {
                j.at("dsn").get_to(pg_dsn); // alias для совместимости с user.json
            }
        }

        // ENV overrides для клиента: MSG5_SERVER_PORT (в BasicConfig), MSG5_PG_DSN
        void apply_env_overrides(const char* prefix = "MSG5_") {
            BasicConfig::apply_env_overrides(prefix);
            const std::string k = std::string(prefix) + "PG_DSN";
            if (const char* v = std::getenv(k.c_str())) if (*v) pg_dsn = v;
        }

        void validate() const {
            BasicConfig::validate();
            // pg_dsn необязателен
        }
    };

} // namespace msg5::config
