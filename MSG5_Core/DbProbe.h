#pragma once
#include <string>

class PgExecutor;

namespace msg5::dbprobe {

	// Существует ли БД (через bootstrap-соединение к кластеру)
	bool database_exists(PgExecutor& boot, const std::string& dbname);

	// Пустая ли прикладная БД (нет пользовательских объектов)
	bool database_empty(PgExecutor& app);

	// Есть ли маркер применённой структуры метаданных
	bool meta_schema_present(PgExecutor& app);

	// Есть ли маркер первичной загрузки метаданных
	bool meta_seed_present(PgExecutor& app);

} // namespace msg5::dbprobe
