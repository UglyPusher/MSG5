// MSG5_Core/Config/BasicConfig.cpp
#include "pch.h"
#include "BasicConfig.h"
#include "../Utils/Env.h"  // getenv_str(...)
#include <stdexcept>

namespace msg5::config {

    void BasicConfig::apply_env_overrides(const char* prefix) {
        // <PREFIX>SERVER_PORT
        std::string k = std::string(prefix) + "SERVER_PORT";
        if (auto v = msg5::utils::getenv_str(k.c_str())) {
            try { server_port = std::stoi(*v); }
            catch (...) {}
        }
    }

    void BasicConfig::validate() const {
        if (server_port <= 0 || server_port > 65535)
            throw std::runtime_error("server_port out of range");
    }

} // namespace msg5::config
