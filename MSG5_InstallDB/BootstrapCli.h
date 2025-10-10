#pragma once
//int RunBootstrapCli(int argc, char** argv);

// Подключаем MSG5_Config для по-командного парсинга
#include "msg5/config/CommandSpec.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/Sources.h"
#include "msg5/config/Resolver.h"

// Единые ключи (внутренние имена опций)
namespace bootstrap {
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
	inline constexpr const char* K_TX_MODE = "tx_mode";           // per-file|single|none
	inline constexpr const char* K_CONT_ON_ERR = "continue_on_error";
	inline constexpr const char* K_DRY_RUN = "dry_run";
	inline constexpr const char* K_FORCE = "force";
	inline constexpr const char* K_YES = "yes";
	
	// По-командные спеки и резолвер
	msg5::config::CommandSpec   MakeSpec_Validate();
	msg5::config::CommandSpec   MakeSpec_CreateDatabase();
	msg5::config::CommandSpec   MakeSpec_ApplyMetaStructure();
	msg5::config::CommandSpec   MakeSpec_ApplyMetaData();

	msg5::config::ResolvedOptions ResolveFor(int argc, const char* const* argv,
		const msg5::config::CommandSpec & spec);
}

int HandleValidate(int argc, char** argv);

int HandleCreateDatabase(int argc, char** argv);

int HandleApplyMetaStructure(int argc, char** argv);

int HandleApplyMetaData(int argc, char** argv);
