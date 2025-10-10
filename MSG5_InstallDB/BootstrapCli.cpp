#include <optional>
#include <filesystem>
#include <cstdlib>
#include <string_view>
#include <fstream>


#include "BootstrapCli.h"

// DB helpers
#include "PgExecutor.h"
#include "DbProbe.h"
#include "Utils/SqlUtil.h"            // quote_ident / quote_lit
#include "bootstrap/Prompts.h"  // confirm_yes / prompt_secret
#include "Utils/FS.h"                 // list_files_with_extension
#include "bootstrap/UsageText.h"

using namespace msg5::config;

namespace {
	std::vector<std::string> TxEnums() { return { "per-file","single","none" }; }
	
	// Safe getenv: MSVC -> _dupenv_s, иначе стандартный getenv
	static std::optional<std::string> GetEnv(std::string_view name) {
#if defined(_MSC_VER)
		char* buf = nullptr;
		size_t len = 0;
		if (_dupenv_s(&buf, &len, std::string(name).c_str()) != 0 || !buf) return std::nullopt;
		std::string v(buf, len ? len - 1 : 0); // len включает нуль-терминатор
		free(buf);
		if (v.empty()) return std::nullopt;
		return v;
#else
		if (const char* p = std::getenv(std::string(name).c_str())) {
			if (*p) return std::string(p);
		}
		return std::nullopt;
#endif
	}

	// --config PATH или ENV MSG5_BOOTSTRAP_CONFIG
	std::optional<std::filesystem::path> FindConfigPath(int argc, const char* const* argv) {
		for (int i = 1; i + 1 < argc; ++i) {
			if (std::string_view(argv[i]) == "--config") {
				return std::filesystem::path(argv[i + 1]);
			}
		}
		if (auto env = GetEnv("MSG5_BOOTSTRAP_CONFIG"); env && !env->empty())
			return std::filesystem::path(*env);
		return std::nullopt;
	}
	
	
	// Источники по приоритету: CLI > FILE(cfg) > ENV(MSG5_*) > PROMPT
	std::vector<IOptionsSourcePtr> MakeDefaultSources(int argc, const char* const* argv,
		std::optional<std::filesystem::path> cfg) {
		std::vector<IOptionsSourcePtr> s;
		s.push_back(makeArgsSource(argc, argv, LogLevel::Info));
		if (cfg) s.push_back(makeFileSource(*cfg, LogLevel::Info));
		s.push_back(makeEnvSource("MSG5_", LogLevel::Info));
		s.push_back(makePromptSource(true, LogLevel::Info));
		return s;
	}
}

namespace bootstrap {
	
	CommandSpec MakeSpec_Validate() {
		CommandSpec cs{};
		cs.name = "validate";
		cs.options = {
			{ K_DSN, OptionType::String, false, OptionFlags::OptNone,
			{"--dsn"}, {"MSG5_BOOTSTRAP_DSN"}, "dsn", "DSN: ", {}, std::nullopt },
			{ K_CONFIG_PATH, OptionType::Path, false, OptionFlags::OptNone,
			{"--config"}, {"MSG5_BOOTSTRAP_CONFIG"}, "config", "Config path: ", {}, std::nullopt },
			{ K_ASK_PASS, OptionType::Bool, false, OptionFlags::OptNone,
			{"--ask-pass"}, {}, "ask_pass", "", {}, std::nullopt },
		};
		return cs;
	}
	
	CommandSpec MakeSpec_CreateDatabase() {
		CommandSpec cs{};
		cs.name = "create-database";
		cs.options = {
			{ K_BOOTSTRAP_DSN, OptionType::String, false, OptionFlags::OptNone, 
			{"--bootstrap-dsn"}, {"MSG5_BOOTSTRAP_DSN"}, "bootstrap_dsn", "Bootstrap DSN: ", {}, std::nullopt },
			{ K_CONFIG_PATH, OptionType::Path, false, OptionFlags::OptNone, 
			{"--config"}, {"MSG5_BOOTSTRAP_CONFIG"}, "config", "Config path: ", {}, std::nullopt },
		
			{ K_DBNAME, OptionType::String, false, OptionFlags::OptNone, 
			{"--dbname"}, {}, "dbname", "DB name: ", {}, std::string("MSG5") },
			{ K_OWNER,  OptionType::String, false, OptionFlags::OptNone, 
			{"--owner"}, {}, "owner", "Owner role: ", {}, std::string("msg5_app_owner") },
			{ K_OWNER_PASS, OptionType::String, false, OptionFlags::OptSecret,
			{"--owner-pass"}, {}, "owner_pass", "Owner password: ", {}, std::nullopt },
			{ K_ASK_OWNER_PASS, OptionType::Bool, false, OptionFlags::OptNone,
			{"--ask-owner-pass"}, {}, "ask_owner_pass", "", {}, std::nullopt },
			{ K_ENCODING, OptionType::String, false, OptionFlags::OptNone,
			{"--encoding"}, {}, "encoding", "Encoding: ", {}, std::string("UTF8") },
			{ K_TEMPLATE, OptionType::String, false, OptionFlags::OptNone,
			{"--template"}, {}, "template", "Template DB: ", {}, std::string("template1") },
		
			{ K_DRY_RUN, OptionType::Bool, false, OptionFlags::OptNone,
			{"--dry-run"}, {}, "dry_run", "", {}, std::nullopt },
			{ K_FORCE,   OptionType::Bool, false, OptionFlags::OptNone,
			{"--force"}, {}, "force", "", {}, std::nullopt },
			{ K_YES,     OptionType::Bool, false, OptionFlags::OptNone,
			{"--yes"}, {}, "yes", "", {}, std::nullopt },
		};
		return cs;
	}
	
	CommandSpec MakeSpec_ApplyMetaStructure() {
		CommandSpec cs{};
		cs.name = "apply-meta-structure";
		cs.options = {
		{ K_APP_DSN, OptionType::String, false, OptionFlags::OptNone,
		{"--app-dsn"}, {"MSG5_APP_DSN"}, "app_dsn", "App DSN: ", {}, std::nullopt },
		{ K_BASELINE_DIR, OptionType::Path, true, OptionFlags::OptNone,
		{"--baseline-dir"}, {}, "baseline_dir", "Baseline dir: ", {}, std::nullopt },
		{ K_CONFIG_PATH, OptionType::Path, false, OptionFlags::OptNone,
		{"--config"}, {"MSG5_BOOTSTRAP_CONFIG"}, "config", "Config path: ", {}, std::nullopt },
		
		{ K_TX_MODE, OptionType::Enum, false, OptionFlags::OptNone,
		{"--tx-mode"}, {}, "tx_mode", "", TxEnums(), std::string("per-file") },
		{ K_CONT_ON_ERR, OptionType::Bool, false, OptionFlags::OptNone,
		{"--continue-on-error"}, {}, "continue_on_error", "", {}, std::nullopt },
		
		{ K_DRY_RUN, OptionType::Bool, false, OptionFlags::OptNone,
		{"--dry-run"}, {}, "dry_run", "", {}, std::nullopt },
		{ K_FORCE,   OptionType::Bool, false, OptionFlags::OptNone,
		{"--force"}, {}, "force", "", {}, std::nullopt },
		{ K_YES,     OptionType::Bool, false, OptionFlags::OptNone,
		{"--yes"}, {}, "yes", "", {}, std::nullopt },
		};
		return cs;
	}

	CommandSpec MakeSpec_ApplyMetaData() {
		CommandSpec cs{};
		cs.name = "apply-meta-data";
		cs.options = {
		{ K_APP_DSN,   OptionType::String, false, OptionFlags::OptNone,
		{"--app-dsn"}, {"MSG5_APP_DSN"}, "app_dsn", "App DSN: ", {}, std::nullopt },
		{ K_DATA_DIR,  OptionType::Path,   true,  OptionFlags::OptNone,
		{"--data-dir"}, {}, "data_dir", "Data dir: ", {}, std::nullopt },
		{ K_CONFIG_PATH, OptionType::Path, false, OptionFlags::OptNone,
		{"--config"}, {"MSG5_BOOTSTRAP_CONFIG"}, "config", "Config path: ", {}, std::nullopt },
		
		{ K_TX_MODE, OptionType::Enum, false, OptionFlags::OptNone,
		{"--tx-mode"}, {}, "tx_mode", "", TxEnums(), std::string("per-file") },
		{ K_CONT_ON_ERR, OptionType::Bool, false, OptionFlags::OptNone,
		{"--continue-on-error"}, {}, "continue_on_error", "", {}, std::nullopt },
		
		{ K_DRY_RUN, OptionType::Bool, false, OptionFlags::OptNone,
		{"--dry-run"}, {}, "dry_run", "", {}, std::nullopt },
		{ K_FORCE,   OptionType::Bool, false, OptionFlags::OptNone,
		{"--force"}, {}, "force", "", {}, std::nullopt },
		{ K_YES,     OptionType::Bool, false, OptionFlags::OptNone,
		{"--yes"}, {}, "yes", "", {}, std::nullopt },
		};
		return cs;
	}
	
	ResolvedOptions ResolveFor(int argc, const char* const* argv, const CommandSpec & spec) {
		auto cfg = FindConfigPath(argc, argv);
		Resolver r{ MakeDefaultSources(argc, argv, cfg) };
		r.set_min_level(LogLevel::Info);
		ResolveParams rp;
		rp.apply_defaults = true;
		rp.validate = true;
		rp.log_validation = true;
		return r.resolve(spec, rp);
	}

} // namespace bootstrap

int HandleValidate(int argc, char** argv) {
	// 1) Разобрать опции этой команды
	auto ro = bootstrap::ResolveFor(argc, const_cast<const char* const*>(argv),
		bootstrap::MakeSpec_Validate());
	const bool ask_pass = ro.values.count(bootstrap::K_ASK_PASS) &&
		ro.values.at(bootstrap::K_ASK_PASS) == "true";
	std::string dsn;
	if (auto it = ro.values.find(bootstrap::K_DSN); it != ro.values.end()) {
		dsn = it->second;
	}
	
	if (dsn.empty()) {
		std::cerr << "[validate] DSN is required. Use --dsn or set MSG5_BOOTSTRAP_DSN.\n";
		bootstrap::print_usage();
		return 2;
	}
	
	// 2) Подключение и проверки
	try {
		if (ask_pass) {
			std::cout << "[validate] --ask-pass specified (PROMPT source supported). If DSN lacks password, prompt source would be used.\n";
		}
		
		PgExecutor db{ dsn };
		std::cout << "[validate] Connected OK.\n";
		
		// Пустая ли БД?
		bool is_empty = msg5::dbprobe::database_empty(db);
		std::cout << "[validate] database_empty: " << (is_empty ? "true" : "false") << "\n";
		
		// Есть ли schema meta?
		bool meta_present = msg5::dbprobe::meta_schema_present(db);
		std::cout << "[validate] meta_schema_present: " << (meta_present ? "true" : "false") << "\n";
		
		// Применён ли seed (метаданные)?
		bool seed_present = msg5::dbprobe::meta_seed_present(db);
		std::cout << "[validate] meta_seed_present: " << (seed_present ? "true" : "false") << "\n";
		
		// Возвращаем 0, но пользователь видит статусы.
		return 0;
	}
	
	catch (const std::exception& ex) {
		std::cerr << "[validate] ERROR: " << ex.what() << "\n";
		return 1;
	}
}

int HandleCreateDatabase(int argc, char** argv) {
	// Разобрать опции create-database
	auto ro = bootstrap::ResolveFor(argc,
		const_cast<const char* const*>(argv), bootstrap::MakeSpec_CreateDatabase());

	// Извлекаем параметры
	const std::string bdsn = ro.values.count(bootstrap::K_BOOTSTRAP_DSN) ? ro.values.at(bootstrap::K_BOOTSTRAP_DSN) : "";
	const std::string dbname = ro.values.count(bootstrap::K_DBNAME) ? ro.values.at(bootstrap::K_DBNAME) : "MSG5";
	const std::string owner = ro.values.count(bootstrap::K_OWNER) ? ro.values.at(bootstrap::K_OWNER) : "msg5_app_owner";
	std::string owner_pass = ro.values.count(bootstrap::K_OWNER_PASS) ? ro.values.at(bootstrap::K_OWNER_PASS) : "";
	const std::string encoding = ro.values.count(bootstrap::K_ENCODING) ? ro.values.at(bootstrap::K_ENCODING) : "UTF8";
	const std::string templ = ro.values.count(bootstrap::K_TEMPLATE) ? ro.values.at(bootstrap::K_TEMPLATE) : "template1";
	const bool ask_owner = ro.values.count(bootstrap::K_ASK_OWNER_PASS) && ro.values.at(bootstrap::K_ASK_OWNER_PASS) == "true";
	const bool dry_run = ro.values.count(bootstrap::K_DRY_RUN) && ro.values.at(bootstrap::K_DRY_RUN) == "true";
	const bool force = ro.values.count(bootstrap::K_FORCE) && ro.values.at(bootstrap::K_FORCE) == "true";
	const bool yes = ro.values.count(bootstrap::K_YES) && ro.values.at(bootstrap::K_YES) == "true";
	
	if (bdsn.empty()) {
		std::cerr << "[create-database] --bootstrap-dsn is required (or ENV MSG5_BOOTSTRAP_DSN).\n";
		bootstrap::print_usage();
		return 2;
	}
	
	if (ask_owner && owner_pass.empty()) {
		owner_pass = bootstrap::prompt_hidden("Owner password: ");
	}
	
	try {
		PgExecutor sys{ bdsn }; // коннект к bootstrap БД (обычно postgres)
		std::cout << "[create-database] Connected to server.\n";
		
		// 1) Проверим, существует ли уже роль и БД
		// Роль:
		{
			std::string sql =
				"select 1 from pg_roles where rolname = " + msg5::sql::quote_lit(owner) + " limit 1";
			bool role_exists = !sys.scalar(sql).empty();
			std::cout << "[plan] role " << owner << (role_exists ? " exists" : " will be created") << "\n";
			
			// 2) План и выполнение
			if (!dry_run && !role_exists) {
				if (!force) {
					std::cout << "[create-database] not forced; nothing executed.\n";
					return 0;
				}
				
				if (!yes && !bootstrap::msg5_prompt_yes_no("Proceed creating role?")) {
					std::cout << "[create-database] canceled.\n";
					return 0;
				}
				
				std::string create_role =
					"do $$ begin "
					"  if not exists (select 1 from pg_roles where rolname = " + msg5::sql::quote_lit(owner) + ") then "
					"    create role " + msg5::sql::quote_ident(owner) + " login password " + msg5::sql::quote_lit(owner_pass) + "; "
					"  end if; "
					"end $$;";
				sys.exec(create_role);
				std::cout << "[ok] role ensured.\n";
			}
		}
		
		// База:
		{
			bool db_exists = msg5::dbprobe::database_exists(sys, dbname);
			std::cout << "[plan] database " << dbname << (db_exists ? " exists" : " will be created") << "\n";
			
			if (dry_run) {
				std::cout << "[dry-run] add --force to execute";
				if (!yes) std::cout << " (and --yes to auto-confirm)";
				std::cout << ".\n";
				return 0;
			}
			
			if (db_exists) {
				std::cout << "[create-database] database already exists, nothing to do.\n";
				return 0;
			}
			
			if (!force) {
				std::cout << "[create-database] not forced; nothing executed.\n";
				return 0;
			}
			
			if (!yes && !bootstrap::msg5_prompt_yes_no("Proceed creating database?")) {
				std::cout << "[create-database] canceled.\n";
				return 0;
			}
			
			// CREATE DATABASE ... OWNER ... ENCODING ... TEMPLATE ...
			std::string sql =
				"create database " + msg5::sql::quote_ident(dbname) +
				" owner " + msg5::sql::quote_ident(owner) +
				" encoding " + msg5::sql::quote_lit(encoding) +
				" template " + msg5::sql::quote_ident(templ) + ";";
			sys.exec(sql);
			std::cout << "[ok] database created.\n";
		}
		
		return 0;
	}
	
	catch (const std::exception& ex) {
		std::cerr << "[create-database] ERROR: " << ex.what() << "\n";
		return 1;
	}
}

int HandleApplyMetaStructure(int argc, char** argv) {
	// 1) Разбор опций
	auto ro = bootstrap::ResolveFor(
		argc, const_cast<const char* const*>(argv),
		bootstrap::MakeSpec_ApplyMetaStructure());
	const std::string app_dsn = ro.values.count(bootstrap::K_APP_DSN) ? ro.values.at(bootstrap::K_APP_DSN) : "";
	const std::string base_dir = ro.values.count(bootstrap::K_BASELINE_DIR) ? ro.values.at(bootstrap::K_BASELINE_DIR) : "";
	const std::string tx_mode = ro.values.count(bootstrap::K_TX_MODE) ? ro.values.at(bootstrap::K_TX_MODE) : "per-file";
	const bool cont_on_err = ro.values.count(bootstrap::K_CONT_ON_ERR) && ro.values.at(bootstrap::K_CONT_ON_ERR) == "true";
	const bool dry_run = ro.values.count(bootstrap::K_DRY_RUN) && ro.values.at(bootstrap::K_DRY_RUN) == "true";
	const bool force = ro.values.count(bootstrap::K_FORCE) && ro.values.at(bootstrap::K_FORCE) == "true";
	const bool yes = ro.values.count(bootstrap::K_YES) && ro.values.at(bootstrap::K_YES) == "true";
	
	if (app_dsn.empty()) {
		std::cerr << "[apply-meta-structure] --app-dsn is required (or ENV MSG5_APP_DSN).\n";
		bootstrap::print_usage();
		return 2;
	}
	
	if (base_dir.empty()) {
		std::cerr << "[apply-meta-structure] --baseline-dir is required.\n";
		bootstrap::print_usage();
		return 2;
	}
	
	try {
		// 2) Скан каталога .sql (верхний уровень; без рекурсии)
		std::vector<std::filesystem::path> files;
		try {
			files = msg5::utils::list_files_with_extension(std::filesystem::path(base_dir), ".sql");
		}

		catch (const std::exception& ex) {
			std::cerr << "[apply-meta-structure] FS error: " << ex.what() << "\n";
			return 2;
		}
		
		if (files.empty()) {
			std::cout << "[apply-meta-structure] No .sql files in: " << base_dir << "\n";
			return 0;
		}
		
		// 3) План
		std::cout << "[plan] tx-mode = " << tx_mode
			<< ", files = " << files.size()
			<< (cont_on_err ? ", continue-on-error" : "") << "\n";
		for (const auto& f : files) {
			std::cout << "  - " << f.string() << "\n";
		}
		
		if (dry_run) {
			std::cout << "[dry-run] add --force to execute";
			if (!yes) std::cout << " (and --yes to auto-confirm)";
			std::cout << ".\n";
			return 0;
		}
		
		if (!force) {
			std::cout << "[apply-meta-structure] not forced; nothing executed.\n";
			return 0;
		}
		
		if (!yes && !bootstrap::msg5_prompt_yes_no("Proceed applying meta structure?")) {
			std::cout << "[apply-meta-structure] canceled.\n";
			return 0;
		}
		
		// 4) Выполнение
		PgExecutor db{ app_dsn };
		auto run_file = [&](const std::filesystem::path& p) -> bool {
				// читаем файл целиком
				std::ifstream in(p, std::ios::binary);
				if (!in) {
					std::cerr << "[apply-meta-structure] cannot open: " << p.string() << "\n";
					return false;
				}
		
				std::string sql((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
				try {
					db.exec(sql);
					std::cout << "[ok] " << p.filename().string() << "\n";
					return true;
				}
		
				catch (const std::exception& ex) {
					std::cerr << "[ERR] " << p.filename().string() << ": " << ex.what() << "\n";
					return false;
				}
			};
		
		if (tx_mode == "single") {
			// один транзакционный блок на все файлы
			try {
				db.exec("begin;");
				bool all_ok = true;
				for (const auto& f : files) {
					if (!run_file(f)) { all_ok = false; if (!cont_on_err) break; }
				}
				
				if (!all_ok && !cont_on_err) {
					db.exec("rollback;");
					std::cerr << "[apply-meta-structure] aborted, rolled back.\n";
					return 1;
				}
				
				db.exec("commit;");
			}

			catch (const std::exception& ex) 
			{
				// если begin/commit упал – откатиться
				try { db.exec("rollback;");
				}
				catch (...) {}
				std::cerr << "[apply-meta-structure] TX error: " << ex.what() << "\n";
				return 1;
			}
		}
		else if (tx_mode == "per-file") {
			// отдельная транзакция на каждый файл
			bool any_err = false;
			for (const auto& f : files) {
			try {
				db.exec("begin;");
				bool ok = run_file(f);
				if (!ok) { any_err = true; if (!cont_on_err) { db.exec("rollback;"); break; } }
				db.exec(ok ? "commit;" : "rollback;");
			}
			catch (const std::exception& ex) {
				any_err = true;
				try { db.exec("rollback;"); }
				catch (...) {}
				std::cerr << "[apply-meta-structure] TX error on file " << f.filename().string()
					<< ": " << ex.what() << "\n";
				if (!cont_on_err) break;
			}
			}
			if (any_err && !cont_on_err) return 1;
		}
		else { // "none"
			bool any_err = false;
			for (const auto& f : files) {
				if (!run_file(f)) { any_err = true; if (!cont_on_err) break; }
			}
			
			if (any_err && !cont_on_err) return 1;
		}
		
		std::cout << "[apply-meta-structure] done.\n";
		return 0;
}
	catch (const std::exception& ex) {
		std::cerr << "[apply-meta-structure] ERROR: " << ex.what() << "\n";
		return 1;
	}
}

int HandleApplyMetaData(int argc, char** argv) {
	// 1) Разбор опций
	auto ro = bootstrap::ResolveFor(
		argc, const_cast<const char* const*>(argv),
		bootstrap::MakeSpec_ApplyMetaData());
	const std::string app_dsn = ro.values.count(bootstrap::K_APP_DSN) ? ro.values.at(bootstrap::K_APP_DSN) : "";
	const std::string data_dir = ro.values.count(bootstrap::K_DATA_DIR) ? ro.values.at(bootstrap::K_DATA_DIR) : "";
	const std::string tx_mode = ro.values.count(bootstrap::K_TX_MODE) ? ro.values.at(bootstrap::K_TX_MODE) : "per-file";
	const bool cont_on_err = ro.values.count(bootstrap::K_CONT_ON_ERR) && ro.values.at(bootstrap::K_CONT_ON_ERR) == "true";
	const bool dry_run = ro.values.count(bootstrap::K_DRY_RUN) && ro.values.at(bootstrap::K_DRY_RUN) == "true";
	const bool force = ro.values.count(bootstrap::K_FORCE) && ro.values.at(bootstrap::K_FORCE) == "true";
	const bool yes = ro.values.count(bootstrap::K_YES) && ro.values.at(bootstrap::K_YES) == "true";
	
	if (app_dsn.empty()) {
		std::cerr << "[apply-meta-data] --app-dsn is required (or ENV MSG5_APP_DSN).\n";
		bootstrap::print_usage();
		return 2;
	}
	
	if (data_dir.empty()) {
		std::cerr << "[apply-meta-data] --data-dir is required.\n";
		bootstrap::print_usage();
		return 2;
	}
	
	try {
		// 2) Скан каталога .sql (верхний уровень; без рекурсии)
		std::vector<std::filesystem::path> files;
		try {
			files = msg5::utils::list_files_with_extension(std::filesystem::path(data_dir), ".sql");
		}
		catch (const std::exception& ex) {
			std::cerr << "[apply-meta-data] FS error: " << ex.what() << "\n";
			return 2;
		}
		if (files.empty()) {
			std::cout << "[apply-meta-data] No .sql files in: " << data_dir << "\n";
			return 0;
		}
		
		// 3) План
		std::cout << "[plan] tx-mode = " << tx_mode
			<< ", files = " << files.size()
			<< (cont_on_err ? ", continue-on-error" : "") << "\n";
		for (const auto& f : files) {
			std::cout << "  - " << f.string() << "\n";
		}
		
		if (dry_run) {
			std::cout << "[dry-run] add --force to execute";
			if (!yes) std::cout << " (and --yes to auto-confirm)";
			std::cout << ".\n";
			return 0;
		}
		
		if (!force) {
			std::cout << "[apply-meta-data] not forced; nothing executed.\n";
			return 0;
		}
		
		if (!yes && !bootstrap::msg5_prompt_yes_no("Proceed applying meta data?")) {
			std::cout << "[apply-meta-data] canceled.\n";
			return 0;
		}
		
		// 4) Выполнение
		PgExecutor db{ app_dsn };
		auto run_file = [&](const std::filesystem::path& p) -> bool {
			std::ifstream in(p, std::ios::binary);
			if (!in) { std::cerr << "[apply-meta-data] cannot open: " << p.string() << "\n"; return false; }
			std::string sql((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			try { db.exec(sql); std::cout << "[ok] " << p.filename().string() << "\n"; return true; }
			catch (const std::exception& ex) {
				std::cerr << "[ERR] " << p.filename().string() << ": " << ex.what() << "\n"; return false;
			}
			};
		
		if (tx_mode == "single") {
			try {
				db.exec("begin;");
				bool all_ok = true;
				for (const auto& f : files) {
					if (!run_file(f)) { all_ok = false; if (!cont_on_err) break; }
				}
				
				if (!all_ok && !cont_on_err) { db.exec("rollback;"); std::cerr << "[apply-meta-data] aborted, rolled back.\n"; return 1; }
				db.exec("commit;");
			}
			
			catch (const std::exception& ex) {
				try { db.exec("rollback;"); }
				catch (...) {}
				std::cerr << "[apply-meta-data] TX error: " << ex.what() << "\n";
				return 1;
			}
		}
		else if (tx_mode == "per-file") {
			bool any_err = false;
			for (const auto& f : files) {
				try {
					db.exec("begin;");
					bool ok = run_file(f);
					db.exec(ok ? "commit;" : "rollback;");
					if (!ok) { any_err = true; if (!cont_on_err) break; }
				}
				catch (const std::exception& ex) {
					try { db.exec("rollback;"); }
					catch (...) {}
					any_err = true;
					std::cerr << "[apply-meta-data] TX error on file " << f.filename().string() << ": " << ex.what() << "\n";
					if (!cont_on_err) break;
				}
			}
			
			if (any_err && !cont_on_err) return 1;
		}
		else { // none
			bool any_err = false;
			for (const auto& f : files) {
				if (!run_file(f)) { any_err = true; if (!cont_on_err) break; }
			}
			if (any_err && !cont_on_err) return 1;
		}
		
		std::cout << "[apply-meta-data] done.\n";
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << "[apply-meta-data] ERROR: " << ex.what() << "\n";
		return 1;
	}
}