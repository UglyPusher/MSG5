#pragma once
#include <nlohmann/json.hpp>
#include <msg5_http.h>

class CoreFacade {
public:
    void route(const std::string& method,
        const httplib::Request& req,
        httplib::Response& res);
    nlohmann::json describeApi() const;
    static std::string version();
};
