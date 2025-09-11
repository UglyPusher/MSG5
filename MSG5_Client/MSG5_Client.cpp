#include "HttpServer.h"
#include "Config/ConfigLoader.h"
#include "ClientConfig.h"
#include "ClientRoutes.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    fs::path cfgPath = msg5::ConfigLoader::ResolvePath(
        argc, argv, "MSG5_CLIENT_CONFIG", ".\\config\\user.json");

    msg5::ConfigLoader loader(cfgPath.string());

    msg5::config::ClientConfig cfg;
    cfg.from_json(loader.root());              // ← ОДИН вызов, базовый уже внутри
    cfg.apply_env_overrides("MSG5_");
    cfg.validate();

    std::cout << "[boot] pid=" <<
#ifdef _WIN32
        static_cast<unsigned long>(::GetCurrentProcessId())
#else
        static_cast<unsigned long>(::getpid())
#endif
        << "\n[boot] cwd=" << fs::current_path().string()
        << "\n[boot] config=" << cfgPath.string()
        << "\n[boot] baseDir=" << cfgPath.parent_path().string()
        << "\n[boot] listen=0.0.0.0:" << cfg.server_port << "\n";;

    HttpServer srv(cfg.server_port);
    srv.initRoutes();
    srv.mountSystemRoutes();
    msg5::client::RegisterClientApiRoutes(srv, cfg);
    srv.start();
}