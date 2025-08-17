#include "pch.h"
#include "HttpServer.h"
#include "ConfigLoader.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        // по умолчанию ищем рядом с решением: MSG5\config\admin.json
        std::string cfgPath = R"(..\..\config\admin.json)";
        if (argc > 1) cfgPath = argv[1];             // можно передать путь как 1-й аргумент

        ConfigLoader cfg(cfgPath);                    // <— НУЖЕН путь
        HttpServer srv(cfg);

        srv.initRoutes();

        srv.subscribe("GET", "/healthz",
            [](const httplib::Request&, httplib::Response& res) {
                res.status = 200; res.set_content("OK", "text/plain");
            });

        srv.subscribe("POST", "/admin/bootstrap/ping",
            [](const httplib::Request&, httplib::Response& res) {
                res.status = 200; res.set_content(R"({"ok":true})", "application/json");
            });

        srv.start();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
