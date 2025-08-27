#if defined(_MSC_VER) && __has_include("pch.h")
#include "pch.h"
#endif

#include "Config/ConfigLoader.h"
#include "AdminConfig.h"
#include "HttpServer.h"
#include "AdminRoutes.h"
#include <iostream>
#include <filesystem>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "AdminPreflight.h" 


int main(int argc, char** argv) {
    try {
        namespace fs = std::filesystem;
        std::ios::sync_with_stdio(false);

        std::cout << "[admin] pid=" <<
#ifdef _WIN32
            static_cast<unsigned long>(::GetCurrentProcessId())
#else
            static_cast<unsigned long>(::getpid())
#endif
            << "\n[admin] cwd=" << fs::current_path().string()
            << "\n";

        // Путь к admin.json:
        // 1) аргумент командной строки
        // 2) ENV MSG5_ADMIN_CONFIG
        // 3) дефолт: ..\..\config\admin.json (относительно x64\Release)
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
        
        if (auto rc = msg5::admin::Preflight(cfg, std::cout, std::cerr);
            rc != msg5::admin::PreflightResult::Ok) {
            return static_cast<int>(rc);
        }

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
