#include "pch.h"
#include "JsonUtil.h"
#include <cstdio>   // std::snprintf

namespace msg5::json {

    std::string Escape(std::string_view s) {
        std::string out;
        out.reserve(s.size() + 8);

        for (unsigned char c : s) {
            switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[7]; // \u00XX
                    std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
                    out += buf;
                }
                else {
                    out.push_back(static_cast<char>(c));
                }
            }
        }
        return out;
    }

} // namespace msg5::json
