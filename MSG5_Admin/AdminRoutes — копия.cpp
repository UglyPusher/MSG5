#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include "AdminRoutes.h"
#include "AdminConfig.h"
#include "HttpServer.h"
#include "PgExecutor.h"
#include <nlohmann/json.hpp>
#include <cstdlib>   // std::getenv
#include <string>

using nlohmann::json;

namespace {

    inline const char* pick_dsn(const std::string& from_cfg, const char* env_name) {
        if (!from_cfg.empty()) return from_cfg.c_str();
        const char* v = std::getenv(env_name);
        return (v && *v) ? v : nullptr;
    }

} // namespace

namespace msg5::admin {

    void RegisterAdminRoutes(HttpServer& srv, const msg5::config::AdminConfig& cfg) {
        // -------- readiness: bootstrap DSN (критичный)
        srv.addReadyCheck(
            "postgres_bootstrap",
            [dsn = cfg.bootstrap_dsn](std::string& msg) -> bool {
                const char* use = pick_dsn(dsn, "MSG5_BOOTSTRAP_DSN");
                if (!use) { msg = "BOOTSTRAP_DSN not configured"; return false; }
                try {
                    PgExecutor pg{ use };
                    auto v = pg.scalar("select 1");
                    if (v != "1") { msg = "unexpected scalar=" + v; return false; }
                    msg = "ok";
                    return true;
                }
                catch (const std::exception& e) {
                    msg = e.what();
                    return false;
                }
            },
            /*critical=*/true
        );

        // -------- readiness: app DSN (некритичный, информативный)
        srv.addReadyCheck(
            "postgres_app",
            [dsn = cfg.app_dsn](std::string& msg) -> bool {
                const char* use = pick_dsn(dsn, "MSG5_APP_DSN");
                if (!use) { msg = "APP_DSN not configured"; return false; }
                try {
                    PgExecutor pg{ use };
                    auto v = pg.scalar("select 1");
                    if (v != "1") { msg = "unexpected scalar=" + v; return false; }
                    msg = "ok";
                    return true;
                }
                catch (const std::exception& e) {
                    msg = e.what();
                    return false;
                }
            },
            /*critical=*/false
        );

        // -------- GET /admin/describe — описание API админки
        srv.subscribe("GET", R"(^/admin/describe$)",
            [&cfg](const httplib::Request&, httplib::Response& res) {
                json j = {
                    {"version", "v1-admin"},
                    {"paths", {
                        "/admin/describe",
                        "/admin/bootstrap/check",
                        "/admin/bootstrap/create-db",
                        "/admin/bootstrap/load-baseline",
                        "/admin/bootstrap/apply-migrations"
                    }},
                    {"dirs", {
                        {"baseline_dir",   cfg.baseline_dir},
                        {"migrations_dir", cfg.migrations_dir}
                    }}
                };
                res.set_content(j.dump(), "application/json; charset=utf-8");
            });

        // -------- POST /admin/bootstrap/check — безопасный пинг обоих DSN-ов
        srv.subscribe("POST", R"(^/admin/bootstrap/check$)",
            [&cfg](const httplib::Request&, httplib::Response& res) {
                auto check_one = [](const char* name, const char* dsn)->json {
                    if (!dsn) return { {"name",name},{"ok",false},{"msg","not configured"} };
                    try {
                        PgExecutor pg{ dsn };
                        auto v = pg.scalar("select version()");
                        return { {"name",name},{"ok",true},{"version",v} };
                    }
                    catch (const std::exception& e) {
                        return { {"name",name},{"ok",false},{"msg",e.what()} };
                    }
                    };
                const char* bootstrap = pick_dsn(cfg.bootstrap_dsn, "MSG5_BOOTSTRAP_DSN");
                const char* app = pick_dsn(cfg.app_dsn, "MSG5_APP_DSN");
                json out = {
                    {"success", true},
                    {"checks", {
                        check_one("bootstrap", bootstrap),
                        check_one("app", app)
                    }}
                };
                res.set_content(out.dump(2), "application/json; charset=utf-8");
            });

        // -------- Заглушки под будущие операции (безопасные, ничего не меняют)
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
