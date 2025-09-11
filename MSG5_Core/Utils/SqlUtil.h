#pragma once
#include <string>
#include <string_view>

namespace msg5::sql {

    // 't'/'1'/'true' → true
    inline bool as_bool(std::string_view v) {
        // normalize to lower
        std::string s; s.reserve(v.size());
        for (char c : v) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        return (s == "t" || s == "true" || s == "1" || s == "on" || s == "yes");
    }

    // "ident" (двойные кавычки удваиваем)
    inline std::string quote_ident(std::string_view s) {
        std::string out; out.reserve(s.size() + 2);
        out.push_back('"');
        for (char c : s) { if (c == '"') out += "\"\""; else out.push_back(c); }
        out.push_back('"');
        return out;
    }

    // 'literal' (одинарные кавычки удваиваем)
    inline std::string quote_lit(std::string_view s) {
        std::string out; out.reserve(s.size() + 2);
        out.push_back('\'');
        for (char c : s) { if (c == '\'') out += "''"; else out.push_back(c); }
        out.push_back('\'');
        return out;
    }

} // namespace msg5::sql
