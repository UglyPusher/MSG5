#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include "AdminPreflight.h"
#include "PgExecutor.h"      // из Core
#include "DbProbe.h"         // из Core
#include <string>

namespace msg5::admin {

    PreflightResult Preflight(const config::AdminConfig& cfg,
        std::ostream& log, std::ostream& err)
    {
        // 1) DSN’ы (после apply_env_overrides в main берём только из cfg)
        const std::string & boot = cfg.bootstrap_dsn;
        const std::string & app = cfg.app_dsn;
        
        if (boot.empty()) {
            err << "[admin] ERROR: bootstrap DSN not configured (AdminConfig.bootstrap_dsn or MSG5_BOOTSTRAP_DSN)\n";
            return PreflightResult::ConfigError;
        }
        if (app.empty()) {
            err << "[admin] ERROR: app DSN not configured (AdminConfig.app_dsn or MSG5_APP_DSN)\n";
            return PreflightResult::ConfigError;
        }

        try {
            // 2) Пингуем кластер и целевую БД
            PgExecutor pgBoot{ boot.c_str() };
            (void)pgBoot.scalar("select 1");

            PgExecutor pgApp{ app.c_str() };
            (void)pgApp.scalar("select 1");

            // 3) Проверяем наличие мета-структуры
            if (!msg5::dbprobe::meta_schema_present(pgApp)) {
                err << "[admin] ERROR: metadata structure is missing (run CLI: apply-meta-structure)\n";
                return PreflightResult::ConfigError;
            }

            // (опционально) можно добавить быстрый self-check ролей в кластере — по желанию.
            log << "[admin] preflight: OK\n";
            return PreflightResult::Ok;
        }
        catch (const std::exception& e) {
            err << "[admin] ERROR: " << e.what() << "\n";
            return PreflightResult::ConnectError;
        }
    }

} // namespace msg5::admin
