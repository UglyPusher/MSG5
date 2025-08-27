#include "pch.h"

#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include "DbProbe.h"
#include "PgExecutor.h"
#include "Utils/SqlUtil.h"

namespace msg5::dbprobe {

    bool database_exists(PgExecutor& boot, const std::string& dbname) {
        using msg5::sql::quote_lit; using msg5::sql::as_bool;
        auto v = boot.scalar(
            "select exists(select 1 from pg_database where datname = " +
            quote_lit(dbname) + ")"
        );
        return as_bool(v);
    }

    bool database_empty(PgExecutor& app) {
        using msg5::sql::as_bool;
        auto v = app.scalar(
            "select not exists ("
            " select 1 from pg_class c join pg_namespace n on n.oid=c.relnamespace"
            " where n.nspname not in ('pg_catalog','information_schema','pg_toast')"
            "   and c.relkind in ('r','v','m','S','f')"
            ")"
        );
        return as_bool(v);
    }

    bool meta_schema_present(PgExecutor& app) {
        using msg5::sql::as_bool;
        auto v = app.scalar("select to_regclass('meta.meta_schema') is not null");
        return as_bool(v);
    }

    bool meta_seed_present(PgExecutor& app) {
        using msg5::sql::as_bool;
        auto v = app.scalar("select to_regclass('meta.meta_seed') is not null");
        return as_bool(v);
    }

} // namespace msg5::dbprobe

