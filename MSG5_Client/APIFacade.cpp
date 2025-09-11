#include "APIFacade.h"
#include <iostream>
#include "SessionContext.h"
#include "PgExecutor.h"
#include "Utils/Env.h"       // msg5::utils::getenv_str

using json = nlohmann::json;

void APIFacade::route(const std::string& method,
    const httplib::Request& req,
    httplib::Response& res) {
    std::string token;

    if (req.has_header("X-Session-Token")) {
        token = req.get_header_value("X-Session-Token");
    }
    else {
        json error = {
            {"success", false},
            {"error_code", "unauthorized"},
            {"message", "Missing session token"},
            {"error_uid", nullptr}
        };
        res.status = 401;
        res.set_content(error.dump(2), "application/json; charset=utf-8");
        return;
    }

    SessionContext context(token);

    std::cout << "[ClientAPIFacade] Routing method: " << method << "\n";

    // Требуем корректный Content-Type, если тело непустое. Пустое тело допускаем как {}.
    if (!req.body.empty()) {
        if (!req.has_header("Content-Type") ||
            req.get_header_value("Content-Type").find("application/json") == std::string::npos) {
            json error = {
            {"success", false},
            {"message", "Unsupported Media Type: expected application/json"},
            {"error_code", "unsupported_media_type"},
            {"error_uid", nullptr}
            };
            res.status = 415;
            res.set_content(error.dump(2), "application/json; charset=utf-8");
            return;
        }
    }
    json input;
    if (req.body.empty()) {
        input = json::object();

    }
    else {
        try {
            input = json::parse(req.body);
        }
        catch ([[maybe_unused]] const std::exception& e) {
            json error = {
            {"success", false},
            {"message", "Invalid JSON"},
            {"error_code", "invalid_json"},
            {"error_uid", nullptr}
            };
            res.status = 400;
            res.set_content(error.dump(2), "application/json; charset=utf-8");
            return;
        }

    }

    // === БИЗНЕС-МЕТОДЫ ===
    if (method == "db_version") {
        // dsn: сначала из фасада (из конфига), иначе из ENV
        auto env_fallback = msg5::utils::getenv_str("MSG5_PG_DSN");
        const std::string use_dsn = !pg_dsn_.empty() ? pg_dsn_ : env_fallback.value_or("");
        if (use_dsn.empty()) {
                json error = {
                    {"success", false},
                    {"error_code", "config_error"},
                    {"message", "MSG5_PG_DSN is not set and no pg_dsn in config"},
                    {"error_uid", nullptr}
                };
                res.status = 500;
                res.set_content(error.dump(2), "application/json; charset=utf-8");
                return;
            }
            try {
                PgExecutor pg{ use_dsn };
                auto ver = pg.scalar("select version()");
                json ok = {
                    {"success", true},
                    {"method", method},
                    {"data", { {"postgres_version", ver} }}
                };
                res.set_content(ok.dump(2), "application/json; charset=utf-8");
                return;
            }
            catch (const std::exception& e) {
                json err = { {"success", false}, {"error", e.what()} };
                res.status = 500;
                res.set_content(err.dump(2), "application/json; charset=utf-8");
                return;
            }
        }
        // По умолчанию — эхо (как и было)
        json response = { {"success", true}, {"method", method}, {"data", input} };
        res.set_content(response.dump(2), "application/json; charset=utf-8");    
}

std::string APIFacade::version() {
    return "v1.0.0-core";  // или GIT_TAG/BUILD_ID
}

nlohmann::json APIFacade::describeApi() const {
    using json = nlohmann::json;

    return {
        {"version", version()},
        {"methods", {
            {"doc_save", "Сохранить документ"},
            {"doc_post", "Провести документ"},
            {"doc_unpost", "Отменить проведение"},
            {"shift_open", "Открыть смену"},
            {"shift_close", "Закрыть смену"},
            {"get_user_profile", "Профиль текущего пользователя"},
            {"db_version", "Версия Postgres (GET /api/db/version, POST /api/v1/db_version)"}
        }},
        {"notes", "Все методы вызываются через POST с JSON-телом"}
    };
}