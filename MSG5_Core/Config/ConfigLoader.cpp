#include "pch.h"
#include "Config/ConfigLoader.h"
#include "Utils/Env.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

namespace msg5 {

    // --- публичные конструкторы/методы ---
    ConfigLoader::ConfigLoader(const std::string& path) {
        loadFromFile(path);
    }

    ConfigLoader::ConfigLoader(int argc, char** argv,
        const char* env_name,
        const fs::path& default_rel)
    {
        auto p = resolvePathImpl(argc, argv, env_name, default_rel);
        loadFromFile(p);
    }

    fs::path ConfigLoader::ResolvePath(int argc, char** argv,
        const char* env_name,
        const fs::path& default_rel)
    {
        return resolvePathImpl(argc, argv, env_name, default_rel);
    }

    // --- приватная реализация резолва пути (внутри класса) ---
    fs::path ConfigLoader::resolvePathImpl(int argc, char** argv,
        const char* env_name,
        const fs::path& default_rel)
    {
        // 1) CLI аргумент
        if (argc >= 2 && argv && argv[1] && argv[1][0]) {
            fs::path p = argv[1];
            if (fs::exists(p)) return p;
            throw std::runtime_error("Config file not found: " + p.string());
        }

        // 2) ENV
        if (env_name && *env_name) {
            if (auto e = utils::getenv_str(env_name)) {
                fs::path p = *e;
                if (fs::exists(p)) return p;
                throw std::runtime_error(std::string(env_name) + " points to missing file: " + p.string());
            }
        }

        // 3) Дефолтный относительный путь от текущей CWD (в VS это x64\\(Debug|Release))
        fs::path p = fs::current_path() / default_rel;
        if (fs::exists(p)) return p;

        throw std::runtime_error(
            "Config path is not provided or file is missing.\n"
            "Pass it as CLI arg, or set " + std::string(env_name ? env_name : "MSG5_CONFIG") +
            ", or place file at: " + p.string());
    }

    // --- остальное ---
    void ConfigLoader::loadFromFile(const fs::path& p) {
        std::ifstream ifs(p, std::ios::binary);
        if (!ifs) throw std::runtime_error("Cannot open config: " + p.string());
        try { ifs >> root_; }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string("Invalid JSON in ") + p.string() + ": " + e.what());
        }
        baseDir_ = p.parent_path();
    }

    bool ConfigLoader::isValid() const noexcept {
        return !root_.is_discarded() && !root_.is_null();
    }

} // namespace msg5
