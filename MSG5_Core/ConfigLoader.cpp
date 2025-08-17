// ConfigLoader.cpp
#include "pch.h"
#include "ConfigLoader.h"
#include <fstream>
#include <iostream>

ConfigLoader::ConfigLoader(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "[ERROR] Failed to open config file\n";
        return;
    }
    try {
        file >> config;
        valid = true;
    }
    catch (...) {
        std::cerr << "[ERROR] Failed to parse config\n";
    }
}

bool ConfigLoader::isValid() const {
    return valid;
}

int ConfigLoader::getInt(const std::string& key, int defaultValue ) const {
    if (config.contains(key) && config[key].is_number_integer()) {
        return config[key].get<int>();
    }

    std::cerr << "[WARN] Config key '" << key
        << "' not found or not an integer. Using default: "
        << defaultValue << std::endl;
    return defaultValue;
}
