#pragma once
class HttpServer;

namespace msg5::config { struct AdminConfig; }

namespace msg5::admin {
	// Регистрирует ручки админ-сервера и readiness-чек(и)
	void RegisterAdminRoutes(HttpServer& srv, const msg5::config::AdminConfig& cfg);
}
