#pragma once
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>

namespace msg5::config::str {

    inline bool starts_with(std::string_view s, std::string_view pref) noexcept {
        return s.size() >= pref.size() && s.compare(0, pref.size(), pref) == 0;
    }

    inline void ltrim_inplace(std::string& s) {
        auto it = std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); });
        s.erase(s.begin(), it);
    }

    inline void rtrim_inplace(std::string& s) {
        auto it = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base();
        s.erase(it, s.end());
    }

    inline void trim_inplace(std::string& s) {
        rtrim_inplace(s);
        ltrim_inplace(s);
    }

    inline std::string trimmed(std::string s) {
        trim_inplace(s);
        return s;
    }

    inline bool iequals(std::string_view a, std::string_view b) noexcept {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            unsigned char ca = static_cast<unsigned char>(a[i]);
            unsigned char cb = static_cast<unsigned char>(b[i]);
            if (std::tolower(ca) != std::tolower(cb)) return false;
        }
        return true;
    }

} // namespace msg5::config::str
