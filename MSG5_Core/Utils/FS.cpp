#include "pch.h"
#include "Utils/FS.h"
#include <algorithm>
#include <cctype>
#include <string>

namespace fs = std::filesystem;

namespace msg5::utils {

std::vector<fs::path>
list_files_with_extension(const fs::path& dir, const char* ext_in) {
    std::vector<fs::path> out;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return out;

    std::string target = (ext_in ? std::string(ext_in) : std::string{});
    std::transform(target.begin(), target.end(), target.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    for (auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        auto p = e.path();
        auto ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (target.empty() || ext == target) out.push_back(p);
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace msg5::utils
