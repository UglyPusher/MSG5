#include "pch.h"
#include "HttpServer.h"
#include <iostream>
#include "json.hpp"  // для nlohmann::json
#include "CoreFacade.h"

using json = nlohmann::json;

void HttpServer::initRoutes() {
    // GET /ping
    subscribe("GET", "/ping", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[REQUEST] GET /ping" << std::endl;
        json resp = { {"success", true}, {"message", "pong"} };
        res.set_content(resp.dump(), "application/json");
        });

    // POST /api/v1/test
    subscribe("POST", "/api/v1/test", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[REQUEST] POST /api/v1/test" << std::endl;

        json input;
        try {
            input = json::parse(req.body);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to parse JSON: " << e.what() << std::endl;
            json error = {
                {"success", false},
                {"message", "invalid JSON"},
                {"error", e.what()}
            };
            res.status = 400;
            res.set_content(error.dump(2), "application/json");
            return;
        }

        json response = {
            {"success", true},
            {"echo", input}
        };
        res.set_content(response.dump(2), "application/json");
        });

    CoreFacade* facade = new CoreFacade();  // можно инжектировать в класс, если нужно

    subscribe("GET", "/api/describe", [=](const httplib::Request& req, httplib::Response& res) {
        json api = facade->describeApi();
        res.set_content(api.dump(2), "application/json");
        });

    subscribe("POST", "/api/v1/documents", [=](const httplib::Request& req, httplib::Response& res) {
        facade->route("doc_save", req, res);
        });

    // 404 fallback
    server.set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[NOT FOUND] " << req.method << " " << req.path << std::endl;
        json error = {
            {"success", false},
            {"message", "route not implemented"}
        };
        res.status = 404;
        res.set_content(error.dump(2), "application/json");
        });
}

HttpServer::HttpServer(const ConfigLoader& config) {
    port = config.getInt("server_port", 8080);
}

void HttpServer::start() {
    std::cout << "Server started at http://localhost:" << port << std::endl;
    server.listen("0.0.0.0", port);
}

void HttpServer::subscribe(const std::string& method,
    const std::string& path,
    std::function<void(const httplib::Request&, httplib::Response&)> handler) {
    if (method == "GET") {
        server.Get(path.c_str(), handler);
    }
    else if (method == "POST") {
        server.Post(path.c_str(), handler);
    }
    else {
        std::cerr << "[WARN] Unsupported HTTP method for path: " << path << std::endl;
    }
}