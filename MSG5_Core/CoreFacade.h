#pragma once
#include "json.hpp"
#include "httplib.h"

class CoreFacade {
public:
    void route(const std::string& method,
        const httplib::Request& req,
        httplib::Response& res);
    nlohmann::json describeApi() const;
    static std::string version();
};
