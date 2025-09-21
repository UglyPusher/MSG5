#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace msg5::config::cli {

	// Простой парсер argv:
	//   --key=value
	//   --key value
	//   -abc     → a=true, b=true, c=true
	// Позиционные аргументы пока игнорируем.
	std::unordered_map<std::string, std::string>
		parse(const std::vector<std::string>& argv);

} // namespace msg5::config::cli
