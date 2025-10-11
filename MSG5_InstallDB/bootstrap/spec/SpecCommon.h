#pragma once
#include <string>
#include <vector>

namespace msg5::bootstrap::spec {
    inline const std::vector<std::string>& TxEnums() {
        static const std::vector<std::string> k = { "per-file","single","none" };
        return k;
    }
} // namespace msg5::bootstrap::spec
