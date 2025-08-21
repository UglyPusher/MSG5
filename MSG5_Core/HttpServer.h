#pragma once
#include "config/ConfigLoader.h"
#include "httplib.h"  // ✔️ заголовочная библиотека

class HttpServer {
public:
    explicit HttpServer(int port);   // ← единственный конструктор


    // системные маршруты ядра
    void mountSystemRoutes();

    // регистрация проверок готовности (используются в /readyz)
    using ReadyCheck = std::function<bool(std::string& msg)>; // true=ok; msg=детали
    void addReadyCheck(std::string name, ReadyCheck fn, bool critical = true);

    void initRoutes();
    void start();

    void setAppId(std::string id) { appId_ = std::move(id); }  // ← добавить

    void subscribe(const std::string& method,
        const std::string& path,
        std::function<void(const httplib::Request&, httplib::Response&)> handler);
    void setPort(int port) { port_ = port; } // опционально

private:
    struct ReadyCheckItem {
        std::string name;
        ReadyCheck  fn;
        bool        critical{ true };
    };

    int port_{ 8080 };
    std::string appId_ = "msg5";  // ← добавить

    std::vector<ReadyCheckItem> readyChecks_;   // ← то самое поле    int port_ = 8080;
    httplib::Server server;
};
