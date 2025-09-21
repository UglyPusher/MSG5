#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace msg5::config {
	// Есть ли переменная в окружении (отличаем «нет» от «пустая строка»)
	bool env_exists(const char* name);
	
	// Безопасно получить значение ENV (пустая строка — валидно; если нет переменной — вернётся "")
	std::string safe_getenv(std::string_view name);

	std::string to_env_key(std::string s);
}