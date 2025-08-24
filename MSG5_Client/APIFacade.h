#pragma once
#include <nlohmann/json.hpp>
#include <msg5_http.h>
#include <string>

class APIFacade {
public:
    explicit APIFacade(std::string pg_dsn = {}) : pg_dsn_(std::move(pg_dsn)) {}
    void route(const std::string& method,
        const httplib::Request& req,
        httplib::Response& res);
    nlohmann::json describeApi() const;
    static std::string version();
private:
    std::string pg_dsn_;
};
