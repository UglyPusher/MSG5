#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include "AdminRoutes.h"
#include "AdminConfig.h"
#include "HttpServer.h"
#include "PgExecutor.h"

#include <nlohmann/json.hpp>
#include <string>

//#include "DbProbe.h"
//#include "Utils/SqlUtil.h"

using nlohmann::json;

namespace {
    // быстрый ping DSN для ready-check'ов и /admin/bootstrap/check
    inline bool ping_dsn(const std::string& dsn, std::string& msg) {
        if (dsn.empty()) { msg = "DSN not configured"; return false; }
        try {
            PgExecutor pg{ dsn.c_str() };
            auto v = pg.scalar("select 1");
            if (v != "1") { msg = "unexpected scalar=" + v; return false; }
            msg = "ok";
            return true;
        }
        catch (const std::exception& e) {
            msg = e.what();
            return false;
        }
    }
} // namespace


namespace msg5::admin {

    void RegisterAdminRoutes(HttpServer& srv, const msg5::config::AdminConfig& cfg) {
        // -------- readiness: bootstrap DSN (критичный)
        srv.addReadyCheck(
            "postgres_bootstrap",
            [dsn = cfg.bootstrap_dsn](std::string& msg) -> bool { return ping_dsn(dsn, msg); },
            /*critical=*/true
        );

        // -------- readiness: app DSN (некритичный)
        srv.addReadyCheck(
            "postgres_app",
            [dsn = cfg.app_dsn](std::string& msg) -> bool { return ping_dsn(dsn, msg); },
            /*critical=*/false
        );

        srv.addReadyCheck(
            "meta_structure",
            [dsn = cfg.app_dsn](std::string& msg) -> bool {
                try {
                    PgExecutor pg{ dsn.c_str() };
                    auto v = pg.scalar("select to_regclass('meta.meta_schema') is not null");
                    if (v == "t" || v == "1" || v == "true") { msg = "ok"; return true; }
                    msg = "meta.meta_schema missing"; return false;
                }
                catch (const std::exception& e) { msg = e.what(); return false; }
            },
            /*critical=*/false
        );

        // -------- GET /admin/describe — описание API
        srv.subscribe("GET", R"(^/admin/describe$)",
            [&cfg](const httplib::Request&, httplib::Response& res) {
                json j = {
                    {"version", "v1-admin"},
                    {"dirs", {
                        {"baseline_dir",   cfg.baseline_dir},
                        {"migrations_dir", cfg.migrations_dir}
                    }},
                };
                j["paths"] = {
                    "/admin/describe",
                    "/admin/bootstrap/check",
                    "/admin/bootstrap/create-db",
                    "/admin/bootstrap/load-baseline",
                    "/admin/bootstrap/apply-migrations"
                };
                res.set_content(j.dump(), "application/json; charset=utf-8");
            });

        // ===== Нормальный режим: безопасные ручки =====

        // Пинг обоих DSN (безопасно)
        srv.subscribe("POST", R"(^/admin/bootstrap/check$)",
            [&cfg](const httplib::Request&, httplib::Response& res) {
                auto check_one = [](const char* name, const std::string& dsn) -> nlohmann::json {
                    if (dsn.empty()) return { {"name",name},{"ok",false},{"msg","not configured"} };
                    try {
                        PgExecutor pg{ dsn.c_str() };
                        auto v = pg.scalar("select version()");
                        return { {"name",name},{"ok",true},{"version",v} };
                    }
                    catch (const std::exception& e) {
                        return { {"name",name},{"ok",false},{"msg",e.what()} };
                    }
                    };

                const std::string& bootstrap = cfg.bootstrap_dsn;
                const std::string& app = cfg.app_dsn;

                nlohmann::json out = {
                    {"success", true},
                    {"checks", {
                        check_one("bootstrap", bootstrap),
                        check_one("app", app)
                    }}
                };
                res.set_content(out.dump(2), "application/json; charset=utf-8");
            });

        // Заглушки (остальные операции реализуем позже)
        auto not_implemented = [](const std::string& path, httplib::Response& res) {
            json j = { {"success",false},
                      {"error_code","not_implemented"},
                      {"message","Endpoint is a stub: " + path} };
            res.status = 501;
            res.set_content(j.dump(2), "application/json; charset=utf-8");
            };

        srv.subscribe("POST", R"(^/admin/bootstrap/create-db$)",
            [not_implemented](const httplib::Request& req, httplib::Response& res) {
                (void)req;
                not_implemented("/admin/bootstrap/create-db", res);
            });

        srv.subscribe("POST", R"(^/admin/bootstrap/load-baseline$)",
            [not_implemented](const httplib::Request& req, httplib::Response& res) {
                (void)req;
                not_implemented("/admin/bootstrap/load-baseline", res);
            });

        srv.subscribe("POST", R"(^/admin/bootstrap/apply-migrations$)",
            [not_implemented](const httplib::Request& req, httplib::Response& res) {
                (void)req;
                not_implemented("/admin/bootstrap/apply-migrations", res);
            });
    }

} // namespace msg5::admin
