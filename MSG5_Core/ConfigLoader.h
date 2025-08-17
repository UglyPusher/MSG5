// ConfigLoader.h
#pragma once
#include <string>
#include "json.hpp"

class ConfigLoader {
public:
    explicit ConfigLoader(const std::string& path);
    int getInt(const std::string& key, int defaultValue = 8080) const;
    bool isValid() const;
private:
    nlohmann::json config;
    bool valid = false;
};
