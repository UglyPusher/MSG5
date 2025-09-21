#include "pch.h"
#include "env_utils.h"
#include <cstdlib>


namespace msg5::config {
    bool env_exists(const char* name) {
    #if defined(_WIN32)
        char* buf = nullptr;
        size_t len = 0;
        if (_dupenv_s(&buf, &len, name) != 0) return false;
        const bool exists = (buf != nullptr);
        if (buf) free(buf);
        return exists;
    #else
        return std::getenv(name) != nullptr;
    #endif
    }

    std::string safe_getenv(std::string_view name) {
    #if defined(_WIN32)
        char* buf = nullptr;
        size_t len = 0;
        if (_dupenv_s(&buf, &len, std::string{ name }.c_str()) != 0 || !buf) return {};
        std::string v(buf);
        free(buf);
        return v; // пустая строка — валидное значение
    #else
        if (const char* v = std::getenv(std::string{ name }.c_str())) return std::string(v);
        return {};
    #endif
    }

    std::string to_env_key(std::string s) {
        for (char& c : s) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isalnum(uc)) {
                c = static_cast<char>(std::toupper(uc));
            }
            else {
                c = '_';
            }
        }
        return s;
    }
} // namespace msg5::config
