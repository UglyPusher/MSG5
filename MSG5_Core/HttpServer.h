//HttpServer.h
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <utility>      // std::move
#include <msg5_http.h>

class HttpServer {
public:
    explicit HttpServer(int port);

    // системные маршруты ядра (/healthz, /livez, /readyz, /status)
    void mountSystemRoutes();

    // регистрация проверок готовности (для /readyz)
    using ReadyCheck = std::function<bool(std::string& msg)>; // true = ok; msg = детали
    void addReadyCheck(std::string name, ReadyCheck fn, bool critical = true);

    void initRoutes();  // проектные/бизнес-маршруты регистрируются снаружи
    void start();

    void setAppId(std::string id) { appId_ = std::move(id); }
    void setPort(int port) { port_ = port; }

    void subscribe(const std::string& method,
        const std::string& path,
        std::function<void(const httplib::Request&, httplib::Response&)> handler);

private:
    struct ReadyCheckItem {
        std::string name;
        ReadyCheck  fn;
        bool        critical{ true };
    };

    int port_{ 8080 };
    std::string appId_{ "msg5" };

    std::vector<ReadyCheckItem> readyChecks_;
    httplib::Server server;
};
