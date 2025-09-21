#pragma once
#include <string>

namespace msg5::config::fs {
	bool file_exists(const std::string& path);
	std::string read_all_text(const std::string& path);
}
