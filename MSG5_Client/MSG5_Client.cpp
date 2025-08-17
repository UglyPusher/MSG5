#include "pch.h"
#include "HttpServer.h"
#include "ConfigLoader.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        std::string cfgPath = R"(..\..\config\user.json)";
        if (argc > 1) cfgPath = argv[1];

        ConfigLoader cfg(cfgPath);        // <— НУЖЕН путь
        HttpServer srv(cfg);

        srv.initRoutes();

        srv.subscribe("GET", "/healthz",
            [](const httplib::Request&, httplib::Response& res) {
                res.status = 200; res.set_content("OK", "text/plain");
            });

        srv.subscribe("GET", "/api/ping",
            [](const httplib::Request&, httplib::Response& res) {
                res.status = 200; res.set_content(R"({"pong":"ok"})", "application/json");
            });

        srv.start();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
