#include "pch.h"
#include "CoreFacade.h"
#include <iostream>
#include "SessionContext.h"

using json = nlohmann::json;

void CoreFacade::route(const std::string& method,
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
        res.set_content(error.dump(2), "application/json");
        return;
    }

    SessionContext context(token);
    
    std::cout << "[CoreFacade] Routing method: " << method << std::endl;

    json input;
    try {
        input = json::parse(req.body);
    }
    catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"message", "Invalid JSON"},
            {"error_code", "invalid_json"},
            {"error_uid", nullptr}
        };
        res.status = 400;
        res.set_content(error.dump(2), "application/json");
        return;
    }

    // Заглушка: возвращаем метод + входные данные
    json response = {
        {"success", true},
        {"method", method},
        {"data", input}
    };
    res.set_content(response.dump(2), "application/json");
}

std::string CoreFacade::version() {
    return "v1.0.0-core";  // или GIT_TAG/BUILD_ID
}

nlohmann::json CoreFacade::describeApi() const {
    using json = nlohmann::json;

    return {
        {"version", version()},
        {"methods", {
            {"doc_save", "Сохранить документ"},
            {"doc_post", "Провести документ"},
            {"doc_unpost", "Отменить проведение"},
            {"shift_open", "Открыть смену"},
            {"shift_close", "Закрыть смену"},
            {"get_user_profile", "Профиль текущего пользователя"}
        }},
        {"notes", "Все методы вызываются через POST с JSON-телом"}
    };
}