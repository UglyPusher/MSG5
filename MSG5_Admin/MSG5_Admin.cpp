#include "Config/ConfigLoader.h"
#include "Config/BasicConfig.h"
#include "HttpServer.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        namespace fs = std::filesystem;
        // 1) Явно резолвим путь, чтобы залогировать его
        fs::path cfgPath = msg5::ConfigLoader::ResolvePath(
            argc, argv,
            "MSG5_ADMIN_CONFIG",
            "..\\..\\config\\admin.json"
        );
        msg5::ConfigLoader raw(argc, argv, "MSG5_ADMIN_CONFIG", "..\\..\\config\\admin.json");

        msg5::config::BasicConfig cfg;
        cfg.from_json(raw.root());
        cfg.apply_env_overrides();
        cfg.validate();

        // 4) Лог старта
        std::cout << "[boot] config=" << cfgPath.string()
            << "\n[boot] baseDir=" << raw.baseDir().string()
            << "\n[boot] listen=0.0.0.0:" << cfg.server_port << "\n";

        HttpServer srv(cfg.server_port);

        srv.setAppId("admin");
        
        srv.initRoutes();
        srv.mountSystemRoutes(); // ← системные

        srv.start();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
