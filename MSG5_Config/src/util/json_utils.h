#pragma once
#include <string>
#include <unordered_map>

namespace msg5::config::json {

	// Парсит ПЛОСКИЙ JSON-объект: {"k":"v","n":123,"b":true,"x":null}
	// Возвращает пары key -> value (как строки).
	// null пропускается.
	std::unordered_map<std::string, std::string> parse_flat_object(const std::string& text);

}
