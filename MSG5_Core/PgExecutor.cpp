#include "pch.h"
#include "PgExecutor.h"

// только «поштучные» заголовки pqxx
#include <pqxx/connection>
#include <pqxx/transaction>   // pqxx::work
#include <pqxx/result>
#include <pqxx/params>        // pqxx::params(...)
#include <pqxx/zview>         // pqxx::zview

#include <stdexcept>
#include <regex>
#include <initializer_list>
#include <utility>

using json = nlohmann::json;

// Совместимая обёртка под разные версии libpqxx
template <typename... Args>
static pqxx::result exec_compat(pqxx::work& tx, const std::string& sql, Args&&... args) {
#if defined(PQXX_VERSION_MAJOR) && \
   (PQXX_VERSION_MAJOR > 7 || (PQXX_VERSION_MAJOR == 7 && PQXX_VERSION_MINOR >= 9))
    // Новые версии: exec(zview, params)
    return tx.exec(pqxx::zview{ sql.c_str() }, pqxx::params(std::forward<Args>(args)...));
#elif defined(PQXX_VERSION_MAJOR) && (PQXX_VERSION_MAJOR >= 7)
    // Старшие 7.x: ещё есть exec_params
    return tx.exec_params(sql, std::forward<Args>(args)...);
#else
    // Совсем старые: parameterized(...)
    auto p = tx.parameterized(sql);
    (void)std::initializer_list<int>{ (p(std::forward<Args>(args)), 0)... };
    return p.exec();
#endif
}

PgExecutor::PgExecutor(const std::string& conninfo) {
    conn = std::make_unique<pqxx::connection>(conninfo);
    if (!conn || !conn->is_open()) {
        throw std::runtime_error("PG connect failed: connection not open");
    }
}

PgExecutor::~PgExecutor() = default;

void PgExecutor::ensureConnected() const {
    if (!conn || !conn->is_open())
        throw std::runtime_error("PG connection is not open");
}

void PgExecutor::ensureSafeFunctionName(const std::string& qn) {
    // Разрешаем только meta.* и sys.*; имя функции — [a-z][a-z0-9_]*
    static const std::regex re(R"(^(meta|sys)\.[a-z][a-z0-9_]*$)");
    if (!std::regex_match(qn, re)) {
        throw std::invalid_argument("Unsafe function name: " + qn);
    }
}

nlohmann::json PgExecutor::callJson(const std::string& qualified_name,
    const json& payload,
    const SessionContext& context) {
    ensureConnected();
    ensureSafeFunctionName(qualified_name);

    pqxx::work tx{ *conn };

    // Проброс контекста через GUC (локально в транзакции)
    exec_compat(
        tx,
        "SELECT set_config('app.user_uid',$1,true), "
        "       set_config('app.role_uid',$2,true), "
        "       set_config('app.shift_uid',$3,true)",
        context.getUserUid(),
        context.getRoleUid(),
        context.getShiftUid()
    );

    // Просим текстовое представление jsonb для стабильного парсинга на клиенте
    const std::string sql = "SELECT " + qualified_name + "($1::jsonb)::text";
    auto r = exec_compat(tx, sql, payload.dump());

    json out = json::object();
    if (!r.empty() && !r[0][0].is_null()) {
        out = json::parse(r[0][0].c_str());
    }

    tx.commit();
    return out;
}

std::string PgExecutor::callText1Int(const std::string& qualified_name,
    int id,
    const SessionContext& context) {
    ensureConnected();
    ensureSafeFunctionName(qualified_name);

    pqxx::work tx{ *conn };

    exec_compat(
        tx,
        "SELECT set_config('app.user_uid',$1,true), "
        "       set_config('app.role_uid',$2,true), "
        "       set_config('app.shift_uid',$3,true)",
        context.getUserUid(),
        context.getRoleUid(),
        context.getShiftUid()
    );

    const std::string sql = "SELECT " + qualified_name + "($1)";
    auto r = exec_compat(tx, sql, id);

    std::string res;
    if (!r.empty() && !r[0][0].is_null())
        res = r[0][0].as<std::string>("");
    else
        res.clear();

    tx.commit();
    return res;
}
