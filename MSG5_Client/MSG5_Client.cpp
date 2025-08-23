#include "HttpServer.h"
#include "Config/ConfigLoader.h"
#include "Config/BasicConfig.h"
#include "ClientRoutes.h"
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    try {
        namespace fs = std::filesystem;

        fs::path cfgPath = msg5::ConfigLoader::ResolvePath(
            argc, argv,
            "MSG5_CLIENT_CONFIG",
            "..\\..\\config\\user.json"
        );

        msg5::ConfigLoader raw(cfgPath.string());

        msg5::config::BasicConfig cfg;
        cfg.from_json(raw.root());
        cfg.apply_env_overrides();
        cfg.validate();

        std::cout << "[boot] config=" << cfgPath.string()
            << "\n[boot] baseDir=" << raw.baseDir().string()
            << "\n[boot] listen=0.0.0.0:" << cfg.server_port << "\n";

        HttpServer srv(cfg.server_port);
        srv.initRoutes();
        srv.mountSystemRoutes();
        msg5::client::RegisterClientApiRoutes(srv);
        srv.start();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
