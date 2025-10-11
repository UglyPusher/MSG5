#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace msg5::bootstrap::commands {

    // Удаляет UTF-8 BOM (EF BB BF) в начале строки, если он есть.
    inline void strip_utf8_bom(std::string& s) {
        if (s.size() >= 3 &&
            static_cast<unsigned char>(s[0]) == 0xEF &&
            static_cast<unsigned char>(s[1]) == 0xBB &&
            static_cast<unsigned char>(s[2]) == 0xBF) {
            s.erase(0, 3);
        }
    }

    // Стабильная сортировка путей по имени файла (ASCII, без locale).
    inline void sort_paths_by_name(std::vector<std::filesystem::path>&files) {
        std::sort(files.begin(), files.end(),
            [](const auto& a, const auto& b) { return a.filename().string() < b.filename().string(); });
    }
    
    // Быстрая проверка, что строка целиком состоит из пробельных символов.
    inline bool is_all_whitespace(const std::string & s) {
        for (unsigned char c : s) {
            if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')) return false;
        }
        return true;
    }
    
    // Чтение текста файла целиком, с удалением BOM. Возвращает false при ошибке открытия.
    inline bool read_file_text(const std::filesystem::path & p, std::string & out) {
        std::ifstream in(p, std::ios::binary);
        if (!in) return false;
        out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        strip_utf8_bom(out);
        return true;
    }
} // namespace msg5::bootstrap::commands
