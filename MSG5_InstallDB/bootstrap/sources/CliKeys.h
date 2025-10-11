#pragma once
namespace msg5::bootstrap {
	// Общие константы для ENV
	inline constexpr const char* K_ENV_PREFIX = "MSG5_";
	inline constexpr const char* K_ENV_CONFIG_PATH = "MSG5_BOOTSTRAP_CONFIG";
	inline constexpr const char* K_ENV_BOOTSTRAP_DSN = "MSG5_BOOTSTRAP_DSN";
	inline constexpr const char* K_ENV_APP_DSN = "MSG5_APP_DSN";

	inline constexpr const char* K_DSN = "dsn";
	inline constexpr const char* K_BOOTSTRAP_DSN = "bootstrap_dsn";
	inline constexpr const char* K_APP_DSN = "app_dsn";
	inline constexpr const char* K_CONFIG_PATH = "config";
	inline constexpr const char* K_ASK_PASS = "ask_pass";
	inline constexpr const char* K_DBNAME = "dbname";
	inline constexpr const char* K_OWNER = "owner";
	inline constexpr const char* K_OWNER_PASS = "owner_pass";
	inline constexpr const char* K_ASK_OWNER_PASS = "ask_owner_pass";
	inline constexpr const char* K_ENCODING = "encoding";
	inline constexpr const char* K_TEMPLATE = "template";
	inline constexpr const char* K_BASELINE_DIR = "baseline_dir";
	inline constexpr const char* K_DATA_DIR = "data_dir";
	inline constexpr const char* K_TX_MODE = "tx_mode";       // per-file|single|none
	inline constexpr const char* K_CONT_ON_ERR = "continue_on_error";
	inline constexpr const char* K_DRY_RUN = "dry_run";
	inline constexpr const char* K_FORCE = "force";
	inline constexpr const char* K_YES = "yes";
} // namespace msg5::bootstrap
