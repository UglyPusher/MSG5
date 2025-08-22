#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>

namespace msg5 {

    class ConfigLoader {
    public:
        explicit ConfigLoader(const std::string& path);

        // Сам ищет конфиг по argv/env/default
        ConfigLoader(int argc, char** argv,
            const char* env_name,
            const std::filesystem::path& default_rel);

        // Опционально: получить только путь (для логов/тестов)
        static std::filesystem::path ResolvePath(int argc, char** argv,
            const char* env_name,
            const std::filesystem::path& default_rel);

        const nlohmann::json& root()    const noexcept { return root_; }
        const std::filesystem::path& baseDir() const noexcept { return baseDir_; }
        bool isValid() const noexcept;

    private:
        // ← весь «ум» по поиску пути теперь тут, внутри класса
        static std::filesystem::path resolvePathImpl(int argc, char** argv,
            const char* env_name,
            const std::filesystem::path& default_rel);

        void loadFromFile(const std::filesystem::path& p);

        nlohmann::json        root_;
        std::filesystem::path baseDir_;
    };

} // namespace msg5
