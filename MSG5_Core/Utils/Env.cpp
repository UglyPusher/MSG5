#include "pch.h"
#include "Env.h"
#include <cstdlib>   // std::getenv
#include <string>

namespace msg5::utils {

    std::optional<std::string> getenv_str(const char* name) {
#ifdef _WIN32
        // Безопасное копирование для Windows (во избежание гонок со статическим буфером)
        char* buf = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&buf, &sz, name) == 0 && buf != nullptr) {
            std::string val(buf);
            free(buf);
            return val;
        }
        return std::nullopt;
#else
        if (const char* v = std::getenv(name)) {
            return std::string(v);
        }
        return std::nullopt;
#endif
    }

} // namespace msg5::utils
