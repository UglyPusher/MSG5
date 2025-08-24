#include "Config/ConfigLoader.h"
#include "AdminConfig.h"
#include "HttpServer.h"
#include "AdminRoutes.h"
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    try {
        namespace fs = std::filesystem;
        // Путь к admin.json:
        // 1) аргумент командной строки
        // 2) ENV MSG5_ADMIN_CONFIG
        // 3) дефолт: ..\..\MSG5_Admin\config\admin.json (относительно x64\Release)
        fs::path cfgPath = msg5::ConfigLoader::ResolvePath(
            argc, argv,
            "MSG5_ADMIN_CONFIG",
            "..\\..\\config\\admin.json"
        );

        msg5::ConfigLoader loader(cfgPath.string());

        msg5::config::AdminConfig cfg;
        cfg.from_json(loader.root());
        cfg.apply_env_overrides("MSG5_");
        cfg.validate();

        std::cout
            << "[admin] config=" << cfgPath.string()
            << "\n[admin] baseDir=" << cfgPath.parent_path().string()
            << "\n[admin] listen=0.0.0.0:" << cfg.server_port
            << "\n[admin] baseline_dir=" << cfg.baseline_dir
            << "\n[admin] migrations_dir=" << cfg.migrations_dir
            << "\n";

        HttpServer srv(cfg.server_port);
        srv.initRoutes();
        srv.mountSystemRoutes();

        msg5::admin::RegisterAdminRoutes(srv, cfg);

        srv.start();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
