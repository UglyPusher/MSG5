#pragma once
#include <nlohmann/json.hpp>

namespace msg5::utils {

// Считать JSON из stdin, если use_stdin=true и stdin не пустой.
// При ошибках парсинга возвращает пустой объект {}.
nlohmann::json load_stdin_json_if_any(bool use_stdin);

} // namespace msg5::utils
