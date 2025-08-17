#pragma once
#include "ConfigLoader.h"
#include "httplib.h"  // ✔️ заголовочная библиотека

class HttpServer {
public:
    explicit HttpServer(const ConfigLoader& config);
    // Инициализация маршрутов
    void initRoutes();

    void start();
    void subscribe(const std::string& method,
        const std::string& path,
        std::function<void(const httplib::Request&, httplib::Response&)> handler);

private:
    int port;
    httplib::Server server;
};
