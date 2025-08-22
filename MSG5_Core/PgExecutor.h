#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "SessionContext.h"

// forward-declare, чтобы не тянуть pqxx в заголовок
namespace pqxx { class connection; }

class PgExecutor {
public:
    // conninfo: "postgresql://user:pass@host:port/dbname" или "host=... dbname=... user=..."
    explicit PgExecutor(const std::string& conninfo);
    ~PgExecutor();

    // Универсальный JSON-вызов: SELECT <schema.fn>($1::jsonb)::jsonb
    nlohmann::json callJson(const std::string& qualified_name,
        const nlohmann::json& payload,
        const SessionContext& context);

    // Текстовый вызов с 1 int-параметром (пример: meta.generate_sp_ddl(:id) -> TEXT)
    std::string callText1Int(const std::string& qualified_name,
        int id,
        const SessionContext& context);

private:
    std::unique_ptr<pqxx::connection> conn;

    void ensureConnected() const;
    static void ensureSafeFunctionName(const std::string& qualified_name);
};
