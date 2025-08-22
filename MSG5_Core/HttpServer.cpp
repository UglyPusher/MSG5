//HttpServer.cpp
#include "pch.h"
#include "HttpServer.h"
#include <iostream>
#include <nlohmann/json.hpp>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using nlohmann::json;

HttpServer::HttpServer(int port)
    : port_(port) {}

void HttpServer::initRoutes() {
    // business-/project routes are registered externally (in EXE)
}

void HttpServer::start() {
    std::cout << "Server started at http://localhost:" << port_ << std::endl;
    server.listen("0.0.0.0", port_);
}

void HttpServer::subscribe(const std::string& method,
    const std::string& path,
    std::function<void(const httplib::Request&, httplib::Response&)> handler) {
    if (method == "GET") {
        server.Get(path.c_str(), std::move(handler));
    }
    else if (method == "POST") {
        server.Post(path.c_str(), std::move(handler));
    }
    else {
        std::cerr << "[WARN] Unsupported HTTP method for path: " << path << std::endl;
    }
}

void HttpServer::addReadyCheck(std::string name, ReadyCheck fn, bool critical) {
    readyChecks_.push_back(ReadyCheckItem{ std::move(name), std::move(fn), critical });
}

void HttpServer::mountSystemRoutes() {
    // liveness
    subscribe("GET", "/healthz",
        [](const httplib::Request&, httplib::Response& res) {
            res.status = 200; res.set_content("OK", "text/plain");
        });

    subscribe("GET", "/livez",
        [](const httplib::Request&, httplib::Response& res) {
            res.status = 200; res.set_content("OK", "text/plain");
        });

    // readiness — агрегируем зарегистрированные проверки
    subscribe("GET", "/readyz",
        [this](const httplib::Request&, httplib::Response& res) {
            json arr = json::array();
            bool all_ok = true, all_critical_ok = true;

            for (const auto& c : readyChecks_) {
                std::string msg;
                bool ok = false;
                try { ok = c.fn(msg); }
                catch (const std::exception& e) { ok = false; msg = e.what(); }

                arr.push_back({ {"name", c.name}, {"ok", ok}, {"critical", c.critical}, {"msg", msg} });
                all_ok &= ok;
                if (c.critical) all_critical_ok &= ok;
            }

            res.status = all_critical_ok ? 200 : 503;
            res.set_content(json{
                {"ok", all_ok},
                {"all_critical_ok", all_critical_ok},
                {"checks", arr}
                }.dump(), "application/json");
        });

    // статус — лёгкий JSON; можно расширить при необходимости
    subscribe("GET", "/status",
        [this](const httplib::Request&, httplib::Response& res) {
#ifdef _WIN32
            int pid = static_cast<int>(::GetCurrentProcessId());
#else
            int pid = static_cast<int>(::getpid());
#endif
            res.status = 200;
            res.set_content(json{
                {"ok", true},
                {"app", appId_},
                {"port", port_},
                {"pid",  pid}
                }.dump(), "application/json");
        });
}
