#include "pch.h"              // если у клиента включён PCH
#include "ClientRoutes.h"
#include "HttpServer.h"
#include "APIFacade.h"
#include <memory>

namespace msg5::client {

    void RegisterClientApiRoutes(HttpServer& srv) {
        auto facade = std::make_shared<APIFacade>();

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

        srv.subscribe("POST", R"(^/api/v1/([a-z_][a-z0-9_]*)$)",
            [facade](const httplib::Request& req, httplib::Response& res) {
                if (req.matches.size() <= 1) { res.status = 404; return; }
                // ssub_match → строка
                std::string method = req.matches[1].str();
                facade->route(method, req, res);
            });
    }

} // namespace msg5::client
