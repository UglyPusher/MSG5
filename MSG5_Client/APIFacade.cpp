#include "APIFacade.h"
#include <iostream>
#include "SessionContext.h"

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

    std::cout << "[ClientAPIFacade] Routing method: " << method << std::endl;

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

    // Заглушка: возвращаем метод + входные данные
    json response = {
        {"success", true},
        {"method", method},
        {"data", input}
    };
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