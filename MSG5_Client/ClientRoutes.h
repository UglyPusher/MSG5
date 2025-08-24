#pragma once
#include "ClientConfig.h"
class HttpServer;

namespace msg5 { namespace config { struct ClientConfig; } }

namespace msg5::client {
    void RegisterClientApiRoutes(HttpServer& srv, const msg5::config::ClientConfig& cfg);
}
