#include "pch.h"
#include "FileSource.h"
#include "../util/fs_utils.h"
#include "../util/json_utils.h"

namespace msg5::config {

    static std::string pick_config_path() {
        // Можешь поменять порядок/имена под себя
        const char* candidates[] = {
          "config/user.json",
          "config.json"
        };
        for (auto* p : candidates) {
            if (fs::file_exists(p)) return std::string(p);
        }
        return {};
    }

    FetchResult FileSource::fetch(const CommandSpec& /*spec*/) {
        FetchResult r;
        auto path = pick_config_path();
        if (path.empty()) return r;

        auto text = fs::read_all_text(path);
        if (text.empty()) return r;

        auto flat = json::parse_flat_object(text);
        for (auto& kvp : flat) {
            r.kv[kvp.first] = kvp.second;
        }
        return r;
    }

}
