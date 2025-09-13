#include "pch.h"
#include "Utils/Json.h"
#include <iostream>
#include <iterator>
#include <string>

using nlohmann::json;

namespace msg5::utils {

json load_stdin_json_if_any(bool use_stdin) {
    if (!use_stdin) return json::object();
    std::istreambuf_iterator<char> it(std::cin.rdbuf());
    std::string buf(it, {});
    if (buf.empty()) return json::object();
    try { return json::parse(buf); }
    catch (...) { return json::object(); }
}

} // namespace msg5::utils
