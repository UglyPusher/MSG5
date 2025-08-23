#pragma once
#include <optional>
#include <string>

namespace msg5::utils {

	// Возвращает значение переменной окружения как std::string,
	// или std::nullopt, если переменная не установлена.
	//
	// Пример:
	//   if (auto v = msg5::utils::getenv_str("MSG5_APP_DSN")) cfg.app_dsn = *v;
	std::optional<std::string> getenv_str(const char* name);

} // namespace msg5::utils
