#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include "AdminRoutes.h"
#include "AdminConfig.h"
#include "HttpServer.h"
#include "PgExecutor.h"

#include <nlohmann/json.hpp>
#include <cstdlib>      // std::getenv
#include <string>
#include <memory>
#include <atomic>

using nlohmann::json;

namespace {

    // DSN из конфига или ENV (ENV перебивает)
    inline const char* pick_dsn(const std::string& from_cfg, const char* env_name) {
        if (!from_cfg.empty()) return from_cfg.c_str();
        const char* v = std::getenv(env_name);
        return (v && *v) ? v : nullptr;
    }

    // Простой парсер key=val из DSN (host=... dbname=... user=...)
    inline std::string extract_kv(std::string_view dsn, std::string key) {
        key += '=';
        auto pos = dsn.find(key);
        if (pos == std::string_view::npos) return {};
        pos += key.size();
        size_t end = dsn.find(' ', pos);
        std::string val = std::string(dsn.substr(pos, end == std::string_view::npos ? dsn.size() - pos : end - pos));
        if (!val.empty() && val.front() == '"' && val.back() == '"' && val.size() >= 2) {
            val = val.substr(1, val.size() - 2);
        }
        return val;
    }

    inline std::string sql_quote(const std::string& s) {
        std::string out; out.reserve(s.size() + 2);
        out.push_back('\'');
        for (char c : s) { if (c == '\'') out.push_back('\''); out.push_back(c); }
        out.push_back('\'');
        return out;
    }

    struct WizardState {
        std::atomic<bool> enabled{ false };
        std::string database;
        std::string owner;
        std::string reason; // почему включён мастер
    };

} // namespace


namespace msg5::admin {

    void RegisterAdminRoutes(HttpServer& srv, const msg5::config::AdminConfig& cfg) {
        // ---------- Вычисляем режим мастера при старте ----------
        auto wiz = std::make_shared<WizardState>();
        const char* boot = pick_dsn(cfg.bootstrap_dsn, "MSG5_BOOTSTRAP_DSN");
        const char* app = pick_dsn(cfg.app_dsn, "MSG5_APP_DSN");

        std::string appdsn = app ? std::string(app) : std::string();
        wiz->database = extract_kv(appdsn, "dbname");
        wiz->owner = extract_kv(appdsn, "user");

        bool db_exists = false;
        if (!boot) {
            wiz->enabled = true;
            wiz->reason = "BOOTSTRAP_DSN not configured";
        }
        else if (!app || wiz->database.empty()) {
            wiz->enabled = true;
            wiz->reason = "APP_DSN not configured or missing dbname";
        }
        else {
            try {
                PgExecutor pg{ boot };
                auto exists = pg.scalar(
                    "select exists(select 1 from pg_database where datname = " +
                    sql_quote(wiz->database) + ")"
                    );
                db_exists = (exists == "t" || exists == "1" || exists == "true");
                wiz->enabled = !db_exists;
                wiz->reason = db_exists ? "" : "target database does not exist";
            }
            catch (const std::exception& e) {
                wiz->enabled = true;
                wiz->reason = std::string("bootstrap check failed: ") + e.what();
            }
        }

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

        // -------- readiness: app DSN (некритичный)
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

        // -------- GET /admin/describe — описание API (+ wizard info)
        srv.subscribe("GET", R"(^/admin/describe$)",
            [&cfg, wiz](const httplib::Request&, httplib::Response& res) {
                json j = {
                    {"version", "v1-admin"},
                    {"dirs", {
                        {"baseline_dir",   cfg.baseline_dir},
                        {"migrations_dir", cfg.migrations_dir}
                    }},
                    {"wizard", {
                        {"enabled",  wiz->enabled.load()},
                        {"database", wiz->database},
                        {"owner",    wiz->owner},
                        {"reason",   wiz->reason}
                    }}
                };
                if (wiz->enabled) {
                    j["paths"] = {
                        "/admin/describe",
                        "/admin/wizard/state",
                        "/admin/wizard/validate"
                    };
                }
                else {
                    j["paths"] = {
                        "/admin/describe",
                        "/admin/bootstrap/check",
                        "/admin/bootstrap/create-db",
                        "/admin/bootstrap/load-baseline",
                        "/admin/bootstrap/apply-migrations"
                    };
                }
                res.set_content(j.dump(), "application/json; charset=utf-8");
            });

        // ===== Маршруты зависят от режима =====
        if (wiz->enabled) {
            // ===== WIZARD-MODE: только state и validate (никаких DDL) =====

            // Состояние мастера
            srv.subscribe("GET", R"(^/admin/wizard/state$)",
                [wiz](const httplib::Request&, httplib::Response& res) {
                    json j = {
                        {"enabled",  wiz->enabled.load()},
                        {"database", wiz->database},
                        {"owner",    wiz->owner},
                        {"reason",   wiz->reason}
                    };
                    res.set_content(j.dump(2), "application/json; charset=utf-8");
                });

            // Проверка DSN/прав/адреса (ничего не меняет)
            srv.subscribe("POST", R"(^/admin/wizard/validate$)",
                [&cfg](const httplib::Request&, httplib::Response& res) {
                    const char* boot = pick_dsn(cfg.bootstrap_dsn, "MSG5_BOOTSTRAP_DSN");
                    const char* app = pick_dsn(cfg.app_dsn, "MSG5_APP_DSN");

                    auto check = [](const char* name, const char* dsn)->json {
                        if (!dsn) return { {"name",name},{"ok",false},{"msg","not configured"} };
                        try {
                            PgExecutor pg{ dsn };
                            json j;
                            j["name"] = name;
                            j["ok"] = true;
                            j["version"] = pg.scalar("select version()");
                            j["server_addr"] = pg.scalar("select inet_server_addr()::text");
                            j["port"] = pg.scalar("select current_setting('port')");
                            j["user"] = pg.scalar("select current_user");
                            j["roles"] = {
                                {"rolsuper",      pg.scalar("select rolsuper from pg_roles where rolname=current_user")},
                                {"rolcreatedb",   pg.scalar("select rolcreatedb from pg_roles where rolname=current_user")},
                                {"rolcreaterole", pg.scalar("select rolcreaterole from pg_roles where rolname=current_user")}
                            };
                            return j;
                        }
                        catch (const std::exception& e) {
                            return json{ {"name",name},{"ok",false},{"msg",e.what()} };
                        }
                        };

                    json out = {
                        {"success", true},
                        {"checks",  { check("bootstrap", boot), check("app", app) }}
                    };
                    res.set_content(out.dump(2), "application/json; charset=utf-8");
                });

            // В wizard-mode остальные «опасные» ручки НЕ регистрируем.
            return;
        }

        // ===== NORMAL-MODE: как было раньше (безопасные ручки доступны) =====

        // Пинг обоих DSN (безопасно)
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
