#include "pch.h"
#include "fs_utils.h"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstdlib>


namespace msg5::config::fs {

    bool file_exists(const std::string& path) {
        struct stat st {};
        return ::stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFREG);
    }

    std::string read_all_text(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return {};
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

}
