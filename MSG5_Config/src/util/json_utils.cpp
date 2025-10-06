#include "pch.h"
#include "json_utils.h"
#include <regex>
#include <string>
#include <unordered_map>

namespace msg5::config::json {

    static std::string unescape(const std::string& s) {
        std::string out; out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '\\' && i + 1 < s.size()) {
                char n = s[++i];
                switch (n) {
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case '"': out.push_back('"');  break;
                case '\\':out.push_back('\\'); break;
                default: out.push_back(n); break;
                }
            }
            else out.push_back(c);
        }
        return out;
    }

    std::unordered_map<std::string, std::string>
        parse_flat_object(const std::string& text) {
        std::unordered_map<std::string, std::string> kv;

        // Удалим обрамляющие пробелы/скобки по-быстрому
        auto l = text.find('{'); auto r = text.rfind('}');
        if (l == std::string::npos || r == std::string::npos || r <= l) return kv;
        std::string body = text.substr(l + 1, r - l - 1);

        // Регекс на пары "key" : ( "str" | number | true | false | null )
        // Простая, но надёжная для плоских случаев
        static const std::regex re(
            R"###("((?:[^"\\]|\\.)*)"\s*:\s*(?:"((?:[^"\\]|\\.)*)"|(-?\d+(?:\.\d+)?)|(true|false)|(null)))###",
            std::regex::optimize);

        auto it = std::sregex_iterator(body.begin(), body.end(), re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            const auto& m = *it;
            std::string key = m.str(1);
            std::string sval = m.str(2);
            std::string num = m.str(3);
            std::string b = m.str(4);
            std::string nul = m.str(5);

            key = unescape(key);
            if (!nul.empty()) continue;
            if (!sval.empty()) {
                kv[key] = unescape(sval);
            }
            else if (!num.empty()) {
                kv[key] = num;
            }
            else if (!b.empty()) {
                kv[key] = (b == "true" ? "true" : "false");
            }
        }
        return kv;
    }

}
