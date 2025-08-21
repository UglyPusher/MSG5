// MSG5_Core/Config/BasicConfig.h
#pragma once
#include "../json.hpp"

namespace msg5::config {

    struct BasicConfig {
        int server_port = 8080;
        void from_json(const nlohmann::json& j) {
            if (j.contains("server_port")) j.at("server_port").get_to(server_port);
        }
        void apply_env_overrides(const char* prefix = "MSG5_");
        void validate() const;
    };

} // namespace msg5::config
