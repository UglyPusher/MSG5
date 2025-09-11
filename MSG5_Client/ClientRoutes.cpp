#include "pch.h"              // если у клиента включён PCH
#include "ClientRoutes.h"
#include "HttpServer.h"
#include "APIFacade.h"
#include "PgExecutor.h"
#include "ClientConfig.h"
#include <memory>
#include "Utils/Env.h"

namespace msg5::config { struct ClientConfig; }
namespace msg5::client {

    void RegisterClientApiRoutes(HttpServer& srv, const msg5::config::ClientConfig& cfg) {
        auto facade = std::make_shared<APIFacade>(cfg.pg_dsn);

        // Критичный readiness-чек Postgres
            srv.addReadyCheck(
                "postgres",
                [dsn = cfg.pg_dsn](std::string& msg) -> bool {
                    auto env_dsn = msg5::utils::getenv_str("MSG5_PG_DSN");
                    const std::string use = !dsn.empty() ? dsn : env_dsn.value_or("");
                    if (use.empty()) { msg = "PG DSN not configured"; return false; }
                     try {
                        PgExecutor pg{ use };
                        auto val = pg.scalar("select 1");
                        if (val != "1") { msg = "unexpected scalar=" + val; return false; }
                        msg = "ok";
                        return true;
                    }
                    catch (const std::exception& e) {
                        msg = e.what();
                        return false;
                    }
                    catch (...) {
                        msg = "unknown error";
                        return false;
                    }
                    },
                /*critical=*/true
                );
        
        // Описание API (без авторизации)
        srv.subscribe("GET", "/api/describe",
            [facade](const httplib::Request&, httplib::Response& res) {
                auto j = facade->describeApi();
                res.status = 200;
                res.set_content(j.dump(), "application/json; charset=utf-8");
            });

        // Пример фасадного метода (нужен X-Session-Token)
        srv.subscribe("POST", "/api/v1/documents",
            [facade](const httplib::Request& req, httplib::Response& res) {
                facade->route("doc_save", req, res);
            });

        // Быстрый пинг клиента
        srv.subscribe("GET", "/api/ping",
            [](const httplib::Request&, httplib::Response& res) {
                res.status = 200;
                res.set_content(R"({"pong":"ok"})", "application/json; charset=utf-8");
            });

        // Лёгкая проверка подключения к Postgres
        srv.subscribe("GET", R"(^/api/db/version$)",
            [cfg](const httplib::Request&, httplib::Response& res) {
                auto env_dsn = msg5::utils::getenv_str("MSG5_PG_DSN");
                const std::string dsn = !cfg.pg_dsn.empty() ? cfg.pg_dsn : env_dsn.value_or("");
                if (dsn.empty()) { res.status = 500; res.set_content(R"({"error":"MSG5_PG_DSN is not set"})", "application/json; charset=utf-8"); return; }
                try {
                    PgExecutor pg{ dsn }; auto ver = pg.scalar("select version()");
                    nlohmann::json out = { {"postgres_version", ver} };
                    res.status = 200; res.set_content(out.dump(), "application/json; charset=utf-8");
                }
                catch (const std::exception& e) { nlohmann::json err = { {"error", e.what()} }; res.status = 500; res.set_content(err.dump(), "application/json; charset=utf-8"); }
            });

        srv.subscribe("POST", R"(^/api/v1/([a-z_][a-z0-9_]*)$)",
            [facade, cfg](const httplib::Request& req, httplib::Response& res) {
                if (req.matches.size() <= 1) { res.status = 404; return; }
                // ssub_match → строка
                std::string method = req.matches[1].str();
                facade->route(method, req, res);
            });
    }

} // namespace msg5::client
