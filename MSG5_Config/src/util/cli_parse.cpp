#include "pch.h"
#include "cli_parse.h"
#include <cctype>
#include <cstring>

using namespace std;

namespace msg5::config::cli {

    static inline bool starts_with(const string& s, const char* pfx) {
        size_t n = strlen(pfx);
        return s.size() >= n && memcmp(s.data(), pfx, n) == 0;
    }

    static inline bool is_flag_like(const string& s) {
        return !s.empty() && s[0] == '-';
    }

    unordered_map<string, string> parse_old(const vector<string>& argv) {
        unordered_map<string, string> kv;
        if (argv.empty()) return kv;

        for (size_t i = 1; i < argv.size(); ++i) { // argv[0] — имя программы
            const string& a = argv[i];

            // Длинные флаги: --key[=value]
            if (starts_with(a, "--")) {
                auto body = a.substr(2);
                auto eq = body.find('=');
                if (eq != string::npos) {
                    auto key = body.substr(0, eq);
                    auto val = body.substr(eq + 1);
                    if (!key.empty()) kv[key] = val;
                    continue;
                }

                // Вид: --key value  ИЛИ просто --key (true)
                const string key = body;
                string val = "true";
                if (i + 1 < argv.size() && !is_flag_like(argv[i + 1])) {
                    val = argv[++i];
                }
                if (!key.empty()) kv[key] = val;
                continue;
            }

            // Короткие флаги пачкой: -abc  → a=true, b=true, c=true
            if (starts_with(a, "-") && a.size() > 1) {
                for (size_t j = 1; j < a.size(); ++j) {
                    char c = a[j];
                    if (!isspace((unsigned char)c)) {
                        string key(1, c);
                        kv[key] = "true";
                    }
                }
                continue;
            }

            // Позиционные аргументы сейчас пропускаем (можно добавить позже)
        }

        return kv;
    }

} // namespace msg5::config::cli
