#include "BootstrapCli.h"

#include "PgExecutor.h"
#include "DbProbe.h"
#include "Utils/SqlUtil.h"
#include "Utils/Env.h"
#include "Config/ConfigLoader.h"
#include "bootstrap/Prompts.h"
#include "bootstrap/UsageText.h"


#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>

#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

#include <stdexcept>

// -----------------------------------------------------------------------------
// P0.2 — Safe SQL quoting helpers
//  - sql_quote_ident(s): quotes PostgreSQL identifiers safely.
//  - sql_quote_lit(s):   quotes PostgreSQL string literals safely.
//
// Notes:
//  * Identifiers are double-quoted; inner `"` are doubled.
//  * Literals are single-quoted; inner `'` are doubled; backslashes are not special.
//  * Use for: database names, schema/table/role names, and any dynamic identifier.
//  * DO NOT use ident quoting for connection strings (DSN) — only for SQL text.
// -----------------------------------------------------------------------------
// moved to header


// moved to header
using msg5::utils::getenv_str;
using msg5::sql::quote_lit;
using msg5::sql::quote_ident;


// Convenience builder for CREATE DATABASE. You may use it to avoid manual string concatenation.
// Optional args: owner, template, encoding — pass empty string to skip.
static std::string build_create_database_sql(const std::string & dbname,
    const std::string & owner,
    const std::string & templ,
    const std::string & encoding) {
    if (dbname.empty()) throw std::invalid_argument("CREATE DATABASE: dbname is required");
    std::string sql = "CREATE DATABASE " + quote_ident(dbname);
    if (!owner.empty())    sql += " OWNER " + quote_ident(owner);
    if (!templ.empty())    sql += " TEMPLATE " + quote_ident(templ);
    if (!encoding.empty()) sql += " ENCODING " + quote_lit(encoding);
    sql += ";";
    return sql;
}

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using nlohmann::json;
namespace fs = std::filesystem;
using bootstrap::prompt_line;
using bootstrap::prompt_hidden;
using bootstrap::msg5_prompt_yes_no;
using bootstrap::print_usage;



namespace {

    // -------------------------- утилиты ввода/вывода --------------------------

    // moved to header

// moved to header (prompt_line)
// moved to header


    // печать справки
   // help text (ASCII/UTF-8 safe)
    // moved to header


    // чтение JSON из файла, если он есть
    json load_config_json_if_any(const fs::path& config_path, bool& has_file_out) {
        has_file_out = false;
        if (config_path.empty()) return json::object();
        try {
            msg5::ConfigLoader loader(config_path.string());
            has_file_out = loader.isValid();
            return loader.root();
        }
        catch (...) {
            return json::object();
        }
    }

    // чтение JSON из stdin, если флаг включён
    json load_stdin_json_if_any(bool use_stdin) {
        if (!use_stdin) return json::object();
        std::istreambuf_iterator<char> it(std::cin.rdbuf());
        std::string buf(it, {});
        if (buf.empty()) return json::object();
        try { return json::parse(buf); }
        catch (...) { return json::object(); }
    }

    // -------------------------- структура входных параметров --------------------------

    struct Inputs {
            // global flags
        bool dry_run = true;   // plan-only by default
        bool force = false;    // execute without interactive prompt
        bool assume_yes = false; // auto-confirm interactive prompts
        bool continue_on_error = false; // keep going on SQL file errors

        enum class TxMode { PerFile, Single, None };
        TxMode tx_mode = TxMode::PerFile; // default: atomic per file
        
            // validate
        std::string dsn;
        bool ask_pass = false;

        // create-database
        std::string bootstrap_dsn;
        std::string host, port;
        std::string dbname, owner, owner_pass;
        std::string encoding = "UTF8", templ = "template1";
        bool ask_owner_pass = false;

        // apply-meta-structure
        std::string app_dsn;
        fs::path baseline_dir;

        // infra
        bool stdin_json = false;
        fs::path config_path;
    };
    
    // -----------------------------------------------------------------------------
    // P0.1b legacy bridge for `confirm` (to be removed once all call-sites updated)
    // - We keep a global `confirm` symbol to avoid breaking legacy `if (confirm == "...")` checks.
    // - New behavior is governed by flags: --dry-run / --force / --yes.
    // - Use helpers below in new code paths instead of touching `confirm` directly.
    // -----------------------------------------------------------------------------
    static std::string confirm; // DEPRECATED: do not use in new code

    // Simple prompt function for interactive confirmation; returns true when user answers yes.
    
    // Use this in new code to decide whether an operation should execute.
    // Contract:
    //   - returns false in dry-run;
    //   - returns true if --force or --yes is set;
    //   - otherwise asks the user interactively.
    static bool msg5_should_execute(const std::string& op_name, const Inputs& in) {
        if (in.dry_run) return false;
        if (in.force || in.assume_yes) return true;
        return msg5_prompt_yes_no(("Proceed with " + op_name + "?").c_str());

    }
    // For legacy sites still branching on `confirm == "...")`, we expose a helper that
    // sets `confirm = "ALL"` when execution is allowed non-interactively; otherwise clears it.
    // This preserves old behavior until those sites are refactored to msg5_should_execute().
    static void msg5_legacy_confirm_sync(const Inputs& in) {
        if (in.dry_run) { confirm.clear(); return; }
        if (in.force || in.assume_yes) { confirm = "ALL"; return; }
        confirm.clear();
    }

    // -------------------------- подтверждение выполнения --------------------------
       
    bool confirm_proceed(const char* what, bool assume_yes) {
        if (assume_yes) return true;
        std::cout << "[confirm] Proceed with " << what << "? [y/N]: ";
        std::string line;
        if (!std::getline(std::cin, line)) return false;
        for (auto& c : line) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return (line == "y" || line == "yes");
    }
    
    // -------------------------- слияние параметров из JSON --------------------------

    void merge_validate(const json& j, Inputs& in) {
        auto get = [&](const char* k, std::string& dst) {
            if (j.contains(k) && j.at(k).is_string() && dst.empty())
                dst = j.at(k).get<std::string>();
            };
        get("dsn", in.dsn);
        // совместимость с create-database: можно переиспользовать поля
        get("host", in.host);
        get("port", in.port);
        get("user", in.owner);      // не обязательно
        get("password", in.owner_pass);
    }

    void merge_create_db(const json& j, Inputs& in) {
        auto get = [&](const char* k, std::string& dst) {
            if (j.contains(k) && j.at(k).is_string() && dst.empty())
                dst = j.at(k).get<std::string>();
            };
        get("bootstrap_dsn", in.bootstrap_dsn);
        get("host", in.host);
        get("port", in.port);
        get("dbname", in.dbname);
        get("owner", in.owner);
        get("owner_pass", in.owner_pass);
        get("encoding", in.encoding);
        get("template", in.templ);
    }

    void merge_apply_meta(const json& j, Inputs& in) {
        auto get = [&](const char* k, std::string& dst) {
            if (j.contains(k) && j.at(k).is_string() && dst.empty())
                dst = j.at(k).get<std::string>();
            };
        get("app_dsn", in.app_dsn);
        if (j.contains("baseline_dir") && j.at("baseline_dir").is_string() && in.baseline_dir.empty())
            in.baseline_dir = j.at("baseline_dir").get<std::string>();
    }

    // -------------------------- помощь по create-database --------------------------

    bool role_exists(PgExecutor& pg, const std::string& role) {
        auto v = pg.scalar("select exists(select 1 from pg_roles where rolname=" + quote_lit(role) + ")");
        return v == "t" || v == "true" || v == "1" || v == "TRUE";
    }

    bool database_exists_boot(PgExecutor& pg, const std::string& dbname) {
        auto v = pg.scalar("select exists(select 1 from pg_database where datname=" + quote_lit(dbname) + ")");
        return v == "t" || v == "true" || v == "1" || v == "TRUE";
    }

    // -------------------------- validate --------------------------

    int cmd_validate(Inputs in) {
        // validate must never be dry-run by default
        in.dry_run = false;

        // resolve config path: CLI > ENV > default
        fs::path cfg = in.config_path;
        if (cfg.empty()) {
            try {
                cfg = msg5::ConfigLoader::ResolvePath(0, nullptr,
                    "MSG5_BOOTSTRAP_CONFIG",
                    fs::path("..") / ".." / "config" / "bootstrap.json");
            } catch (...) {
                // no config is okay; proceed with empty JSON
            }
        }

        bool has_file = false;
        json jfile = load_config_json_if_any(cfg, has_file);
        json jstdin = load_stdin_json_if_any(in.stdin_json);

        merge_validate(jfile, in);
        merge_validate(jstdin, in);
        // Precedence: CLI > STDIN-JSON > CONFIG > Console
        // Apply STDIN values only if the current field is empty or equals the value from CONFIG file
        {
            auto _ovr = [&](const char* k, std::string& dst){
                if (jstdin.contains(k) && jstdin.at(k).is_string()) {
                    std::string v = jstdin.at(k).get<std::string>();
                    bool came_from_cfg = (jfile.contains(k) && jfile.at(k).is_string() && dst == jfile.at(k).get<std::string>());
                    if (dst.empty() || came_from_cfg) dst = v; // do not override CLI
                }
            };
            _ovr("dsn", in.dsn);
            _ovr("host", in.host);
            _ovr("port", in.port);
            _ovr("user", in.owner);
            _ovr("password", in.owner_pass);
        }

        // STDIN should override values coming from config (but not CLI)
        {
            auto _ovr = [&](const char* k, std::string& dst){
                if (jstdin.contains(k) && jstdin.at(k).is_string()) {
                    std::string v = jstdin.at(k).get<std::string>();
                    bool from_file = (jfile.contains(k) && jfile.at(k).is_string() && dst == jfile.at(k).get<std::string>());
                    if (dst.empty() || from_file) dst = v;
                }
            };
            _ovr("dsn", in.dsn);
            _ovr("host", in.host);
            _ovr("port", in.port);
            _ovr("user", in.owner);
            _ovr("password", in.owner_pass);
        }

        // Dry-run: skip prompts and real connections
        if (in.dry_run) {
            if (in.dsn.empty()) {
                std::cout << "[plan] validate: would prompt for host/port/dbname/user (dry-run)\n";
                return 0;
            } else {
                std::cout << "[plan] validate: would check connection (dry-run)\n";
                return 0;
            }
        }

        if (in.dsn.empty()) {
            // попробуем собрать dsn интерактивно (минимум: host/port/dbname/user/password)
            std::cout << "[validate] DSN is empty - enter parts\n";
            in.host = in.host.empty() ? prompt_line("  host", "127.0.0.1") : in.host;
            in.port = in.port.empty() ? prompt_line("  port", "5432") : in.port;
            std::string db = prompt_line("  dbname", "postgres");
            std::string user = prompt_line("  user", "postgres");
            std::string pass = in.ask_pass ? prompt_hidden("  password") : std::string{};
            std::ostringstream os;
            os << "host=" << in.host << " port=" << in.port
                << " dbname=" << db << " user=" << user;
            if (!pass.empty()) os << " password=" << pass;
            in.dsn = os.str();
        }

        try {   
            PgExecutor pg{ in.dsn.c_str() };
            auto ver = pg.scalar("select version()");
            auto ip = pg.scalar("select inet_server_addr()");
            auto pport = pg.scalar("select current_setting('port')");
            auto usr = pg.scalar("select current_user");
            std::cout << "[validate] ok\n"
                << "  version: " << ver << "\n"
                << "  server:  " << ip << ":" << pport << "\n"
                << "  user:    " << usr << "\n";

            // ----- minimal technical validation (schemas + owners) -----
            int fails = 0;
            auto ok   = [&](const std::string& what){ std::cout << "[ok]    " << what << "\n"; };
            auto fail = [&](const std::string& what){ std::cout << "[fail]  " << what << "\n"; ++fails; };

            auto check_schema_with_owner = [&](const char* sname){
                try {
                    auto present = pg.scalar(
                        "select count(*) from information_schema.schemata where schema_name = '" +
                        std::string(sname) + "'"
                    );
                    if (present == "1") {
                        ok(std::string("schema '") + sname + "' present");
                        try {
                            auto owner = pg.scalar(
                                "select nspowner::regrole::text from pg_namespace where nspname = '" +
                                std::string(sname) + "'"
                            );
                            ok(std::string("schema '") + sname + "' owner: " + owner);
                        } catch (...) { fail(std::string("schema '") + sname + "' owner lookup failed"); }
                    } else {
                        fail(std::string("schema '") + sname + "' missing");
                    }
                } catch (...) {
                    fail(std::string("schema '") + sname + "' check error");
                }
            };

            check_schema_with_owner("meta");
            check_schema_with_owner("sys");

            if (fails == 0)
                std::cout << "[validate] summary: OK\n";
            else
                std::cout << "[validate] summary: FAIL (" << fails << " issue(s))\n";

return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "[validate] ERROR: " << e.what() << "\n";
            return 7;
        }
    }

    // -------------------------- create-database --------------------------

    int cmd_create_database(Inputs in) {
        // resolve config path: CLI > ENV > default
        fs::path cfg = in.config_path;
        if (cfg.empty()) {
            try {
                cfg = msg5::ConfigLoader::ResolvePath(0, nullptr,
                    "MSG5_BOOTSTRAP_CONFIG",
                    fs::path("..") / ".." / "config" / "bootstrap.json");
            } catch (...) {
                // no config is okay; proceed with empty JSON
            }
        }

        bool has_file = false;
        json jfile = load_config_json_if_any(cfg, has_file);
        json jstdin = load_stdin_json_if_any(in.stdin_json);

        merge_create_db(jfile, in);
        merge_create_db(jstdin, in);
        // Precedence: CLI > STDIN-JSON > CONFIG > Console
        // Apply STDIN values only if the current field is empty or equals the value from CONFIG file
        {
            auto _ovr = [&](const char* k, std::string& dst){
                if (jstdin.contains(k) && jstdin.at(k).is_string()) {
                    std::string v = jstdin.at(k).get<std::string>();
                    bool came_from_cfg = (jfile.contains(k) && jfile.at(k).is_string() && dst == jfile.at(k).get<std::string>());
                    if (dst.empty() || came_from_cfg) dst = v; // do not override CLI
                }
            };
            _ovr("bootstrap_dsn", in.bootstrap_dsn);
            _ovr("host", in.host);
            _ovr("port", in.port);
            _ovr("dbname", in.dbname);
            _ovr("owner", in.owner);
            _ovr("owner_pass", in.owner_pass);
            _ovr("encoding", in.encoding);
            _ovr("template", in.templ);
        }

        // STDIN should override values coming from config (but not CLI)
        {
            auto _ovr = [&](const char* k, std::string& dst){
                if (jstdin.contains(k) && jstdin.at(k).is_string()) {
                    std::string v = jstdin.at(k).get<std::string>();
                    bool from_file = (jfile.contains(k) && jfile.at(k).is_string() && dst == jfile.at(k).get<std::string>());
                    if (dst.empty() || from_file) dst = v;
                }
            };
            _ovr("bootstrap_dsn", in.bootstrap_dsn);
            _ovr("host", in.host);
            _ovr("port", in.port);
            _ovr("dbname", in.dbname);
            _ovr("owner", in.owner);
            _ovr("owner_pass", in.owner_pass);
            _ovr("encoding", in.encoding);
            _ovr("template", in.templ);
        }

        // простая интерактивная докомплектация
        if (in.bootstrap_dsn.empty()) {
            std::cout << "[create] Enter bootstrap DSN (admin):\n";
            in.bootstrap_dsn = prompt_line("  bootstrap_dsn");
        }
        if (in.dbname.empty()) in.dbname = prompt_line("  dbname", "MSG5");
        if (in.owner.empty())  in.owner = prompt_line("  owner", "msg5_app_owner");
        if (in.ask_owner_pass && in.owner_pass.empty())
            in.owner_pass = prompt_hidden("  owner password");
        if (in.encoding.empty()) in.encoding = "UTF8";
        if (in.templ.empty())    in.templ = "template1";

        if (in.bootstrap_dsn.empty() || in.dbname.empty() || in.owner.empty()) {
            std::cerr << "ERROR: bootstrap_dsn/dbname/owner not specified\n";
            return 2;
        }

        bool dry_run = in.dry_run;

        // Unified execution decision: dry-run blocks, --force/--yes allow, otherwise ask.
        if (!msg5_should_execute("CREATE-DATABASE", in)) {
            std::cout << "DRY-RUN or not confirmed. Use --force or --yes.\n";
            return 1;
        }
        msg5_legacy_confirm_sync(in);

        try {
            PgExecutor boot{ in.bootstrap_dsn.c_str() };

            // ensure role
            if (!role_exists(boot, in.owner)) {
                std::ostringstream sql;
                sql << "CREATE ROLE " << quote_ident(in.owner) << " LOGIN";
                if (!in.owner_pass.empty())
                    sql << " PASSWORD " << quote_lit(in.owner_pass);
                std::cout << (dry_run ? "[plan] " : "[do]  ")
                    << "create role " << in.owner << "\n";
                if (!dry_run) boot.exec(sql.str());
            }
            else {
                std::cout << "[plan] role " << in.owner << " already exists - skip\n";
            }

            // ensure database
            if (!database_exists_boot(boot, in.dbname)) {
                const std::string sql = build_create_database_sql(
                    in.dbname,
                    in.owner,
                    in.templ,
                    in.encoding
                    );
                std::cout << (dry_run ? "[plan] " : "[do]  ")
                    << "create database " << in.dbname
                    << " owner " << in.owner
                    << " encoding " << in.encoding
                    << " template " << in.templ << "\n";
                if (!dry_run) boot.exec(sql);
            }
            else {
                std::cout << "[plan] database " << in.dbname << " already exists - skip\n";
            }

            if (dry_run) {
                std::cout << "\nDRY-RUN complete. To execute, add: --force (or --yes).\n";
            }
            else {
                std::cout << "\nCREATE-DB done.\n";
            }
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 7;
        }
    }

    // -------------------------- apply-meta-structure --------------------------

    std::vector<fs::path> list_sql_files(const fs::path& dir) {
        std::vector<fs::path> out;
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return out;
        for (auto& e : fs::directory_iterator(dir)) {
            if (!e.is_regular_file()) continue;
            auto p = e.path();
            auto ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (ext == ".sql") out.push_back(p);
        }
        std::sort(out.begin(), out.end());
        return out;
    }

    int cmd_apply_meta(Inputs in) {
        // resolve config path: CLI > ENV > default
        fs::path cfg = in.config_path;
        if (cfg.empty()) {
            try {
                cfg = msg5::ConfigLoader::ResolvePath(0, nullptr,
                    "MSG5_BOOTSTRAP_CONFIG",
                    fs::path("..") / ".." / "config" / "bootstrap.json");
            } catch (...) {
                // no config is okay; proceed with empty JSON
            }
        }

        bool has_file = false;
        json jfile = load_config_json_if_any(cfg, has_file);
        json jstdin = load_stdin_json_if_any(in.stdin_json);

        merge_apply_meta(jfile, in);
        merge_apply_meta(jstdin, in);
        // Precedence: CLI > STDIN-JSON > CONFIG > Console
        // Apply STDIN values only if the current field is empty or equals the value from CONFIG file
        {
            auto _ovr = [&](const char* k, std::string& dst){
                if (jstdin.contains(k) && jstdin.at(k).is_string()) {
                    std::string v = jstdin.at(k).get<std::string>();
                    bool came_from_cfg = (jfile.contains(k) && jfile.at(k).is_string() && dst == jfile.at(k).get<std::string>());
                    if (dst.empty() || came_from_cfg) dst = v; // do not override CLI
                }
            };
            _ovr("app_dsn", in.app_dsn);
            if (jstdin.contains("baseline_dir") && jstdin.at("baseline_dir").is_string()) {
                std::string v = jstdin.at("baseline_dir").get<std::string>();
                bool came_from_cfg = (jfile.contains("baseline_dir") && jfile.at("baseline_dir").is_string() && in.baseline_dir == jfile.at("baseline_dir").get<std::string>());
                if (in.baseline_dir.empty() || came_from_cfg) in.baseline_dir = v; // do not override CLI
            }
        }

        // STDIN should override values coming from config (but not CLI)
        {
            auto _ovr = [&](const char* k, std::string& dst){
                if (jstdin.contains(k) && jstdin.at(k).is_string()) {
                    std::string v = jstdin.at(k).get<std::string>();
                    bool from_file = (jfile.contains(k) && jfile.at(k).is_string() && dst == jfile.at(k).get<std::string>());
                    if (dst.empty() || from_file) dst = v;
                }
            };
            _ovr("app_dsn", in.app_dsn);
            if (jstdin.contains("baseline_dir") && jstdin.at("baseline_dir").is_string()) {
                std::string v = jstdin.at("baseline_dir").get<std::string>();
                bool from_file = (jfile.contains("baseline_dir") && jfile.at("baseline_dir").is_string() && in.baseline_dir == jfile.at("baseline_dir").get<std::string>());
                if (in.baseline_dir.empty() || from_file) in.baseline_dir = v;
            }
        }

        // интерактивно доберём недостающее
        if (in.app_dsn.empty()) {
            std::cout << "[apply] Enter app DSN (user that can create meta objects):\n";
            in.app_dsn = prompt_line("  app_dsn");
        }
        if (in.baseline_dir.empty()){
            // Use a portable default like ./db/meta/structure
            const std::string def_base = (fs::path(".") / "db" / "meta" / "structure").string();
            in.baseline_dir = prompt_line("  baseline_dir", def_base);
        }

        if (in.app_dsn.empty() || in.baseline_dir.empty()) {
            std::cerr << "ERROR: app_dsn/baseline_dir not specified\n";
            return 2;
        }

        auto files = list_sql_files(in.baseline_dir);
        if (files.empty()) {
            std::cerr << "ERROR: no *.sql files found in " << in.baseline_dir.string() << "\n";
            return 2;
        }

        bool dry_run = in.dry_run;

        // Unified execution decision: dry-run blocks, --force/--yes allow, otherwise ask.
        if (!msg5_should_execute("APPLY-META", in)) {
            std::cout << "DRY-RUN or not confirmed. Use --force or --yes.\n";
            return 1;
        }
        msg5_legacy_confirm_sync(in);

        try {
            PgExecutor app{ in.app_dsn.c_str() };

            // идемпотентность: если мета уже есть — ничего не делаем
            if (msg5::dbprobe::meta_schema_present(app)) {
                std::cout << "[apply] meta.meta_schema already present — nothing to do\n";
                return 0;
            }

            std::cout << (dry_run ? "[plan] " : "[do]  ")
                << "Applying baseline from: " << in.baseline_dir.string() << "\n";

            auto tx_mode_str = std::string{};
            switch (in.tx_mode) {
            case Inputs::TxMode::PerFile: tx_mode_str = "per-file"; break;
            case Inputs::TxMode::Single:  tx_mode_str = "single";   break;
            case Inputs::TxMode::None:    tx_mode_str = "none";     break;
            }

            std::cout << (dry_run ? "[plan] " : "[do]  ")
                << "Tx mode: " << tx_mode_str
                << (in.continue_on_error ? " (continue-on-error)" : "") << "\n";

            // If user asks for --tx-mode=single with --continue-on-error, warn and degrade to per-file.
            
            if (in.tx_mode == Inputs::TxMode::Single && in.continue_on_error) {
                std::cout << "[warn] --tx-mode=single conflicts with --continue-on-error; "
                    "falling back to per-file\n";
                in.tx_mode = Inputs::TxMode::PerFile;
            }

            if (in.tx_mode == Inputs::TxMode::Single) {
                // One big transaction
                if (!dry_run) app.exec("BEGIN");
                std::size_t idx = 0;
                try {
                    for (auto& f : files) {
                        ++idx;
                        const auto fname = f.filename().string();
                        std::cout << (dry_run ? "[plan] " : "[do]  ")
                            << "[" << idx << "/" << files.size() << "] run " << fname << "\n";
                        if (dry_run) continue;
                        std::ifstream is(f, std::ios::binary);
                        if (!is) throw std::runtime_error("cannot open " + f.string());
                        std::string sql((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
                        if (!sql.empty()) app.exec(sql);
                    }
                    if (!dry_run) app.exec("COMMIT");
                }
                catch (...) {
                    if (!dry_run) {
                        try { app.exec("ROLLBACK"); }
                        catch (...) {}
                    }
                    throw;
                }
            }
            else {
                // Per file or None
                std::size_t idx = 0;
                for (auto& f : files) {
                    ++idx;
                    const auto fname = f.filename().string();
                    std::cout << (dry_run ? "[plan] " : "[do]  ")
                        << "[" << idx << "/" << files.size() << "] run " << fname << "\n";
                    if (dry_run) continue;

                    std::ifstream is(f, std::ios::binary);
                    if (!is) {
                        std::string msg = "cannot open " + f.string();
                        if (in.continue_on_error) {
                            std::cerr << "[error] " << msg << " - skipping due to --continue-on-error\n";
                            continue;
                        }
                        throw std::runtime_error(msg);
                    }
                    
                    std::string sql((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
                    try {
                        if (in.tx_mode == Inputs::TxMode::PerFile && !sql.empty()) app.exec("BEGIN");
                        if (!sql.empty()) app.exec(sql);
                        if (in.tx_mode == Inputs::TxMode::PerFile && !sql.empty()) app.exec("COMMIT");
                    }
                    catch (const std::exception& e) {
                        if (in.tx_mode == Inputs::TxMode::PerFile) {
                            try { app.exec("ROLLBACK"); }
                            catch (...) {}
                        }
                        const auto sz = sql.size();
                        const std::string preview = sql.substr(0, std::min<std::size_t>(sz, 200));
                        std::cerr << "[error] SQL failed in file: " << fname << " (bytes=" << sz << ")\n"
                            << "        message: " << e.what() << "\n"
                            << "        preview: " << preview << (sz > preview.size() ? "..." : "") << "\n";
                        if (in.continue_on_error) {
                            std::cerr << "        skipping due to --continue-on-error\n";
                            continue;
                        }
                        throw;
                    }
                }
            }

            if (dry_run)
                std::cout << "\nDRY-RUN complete. To execute, add: --force\n";
            else
                std::cout << "\nAPPLY-META done.\n";

            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 7;
        }
    }

} // namespace

// -------------------------- вход в CLI --------------------------

int RunBootstrapCli(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 2; }
    std::string cmd = argv[1];

    Inputs in;
    // простой парсер аргументов (без сторонних библиотек)
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];

        // общие
        if (a == "--stdin-json") { in.stdin_json = true; continue; }
        if (a == "--config" && i + 1 < argc) { in.config_path = argv[++i]; continue; }
        if (a == "--dry-run") { in.dry_run = true; continue; }
        if (a == "--force") { in.dry_run = false; in.force = true; continue; }
        if (a == "--yes" || a == "-y" || a == "--assume-yes") { in.assume_yes = true; continue; }
        if (a == "--continue-on-error") { in.continue_on_error = true; continue; }
        if (a == "--tx-mode" && i + 1 < argc) {
            std::string v = argv[++i];
            for (auto& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (v == "per-file" || v == "perfile" || v == "file") in.tx_mode = Inputs::TxMode::PerFile;
            else if (v == "single" || v == "all") in.tx_mode = Inputs::TxMode::Single;
            else if (v == "none" || v == "off") in.tx_mode = Inputs::TxMode::None;
            else { std::cerr << "Unknown --tx-mode value: " << v << "\n"; return 2; }
            continue;
        }



        // validate
        if (a == "--dsn" && i + 1 < argc) { in.dsn = argv[++i]; continue; }
        if (a == "--ask-pass") { in.ask_pass = true; continue; }

        // create-database
        if (a == "--bootstrap-dsn" && i + 1 < argc) { in.bootstrap_dsn = argv[++i]; continue; }
        if (a == "--host" && i + 1 < argc) { in.host = argv[++i]; continue; }
        if (a == "--port" && i + 1 < argc) { in.port = argv[++i]; continue; }
        if (a == "--dbname" && i + 1 < argc) { in.dbname = argv[++i]; continue; }
        if (a == "--owner" && i + 1 < argc) { in.owner = argv[++i]; continue; }
        if (a == "--owner-pass" && i + 1 < argc) { in.owner_pass = argv[++i]; continue; }
        if (a == "--ask-owner-pass") { in.ask_owner_pass = true; continue; }
        if (a == "--encoding" && i + 1 < argc) { in.encoding = argv[++i]; continue; }
        if (a == "--template" && i + 1 < argc) { in.templ = argv[++i]; continue; }

        // apply-meta-structure
        if (a == "--app-dsn" && i + 1 < argc) { in.app_dsn = argv[++i]; continue; }
        if (a == "--baseline-dir" && i + 1 < argc) { in.baseline_dir = argv[++i]; continue; }

        // help
        if (a == "-h" || a == "--help" || a == "/?") { print_usage(); return 0; }

        std::cerr << "Unknown arg: " << a << "\n";
        return 2;
    }

    if (cmd == "validate")               return cmd_validate(in);
    if (cmd == "create-database")        return cmd_create_database(in);
    if (cmd == "apply-meta-structure")   return cmd_apply_meta(in);

    std::cerr << "Unknown command: " << cmd << "\n";
    print_usage();
    return 2;
}
