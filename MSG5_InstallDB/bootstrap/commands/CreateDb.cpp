#include "CreateDb.h"
#include "bootstrap/spec/CreateDbSpec.h"
#include "bootstrap/sources/SourcesChain.h"
#include "PgExecutor.h"
#include "DbProbe.h"
#include "Utils/SqlUtil.h"
#include "bootstrap/ui/Prompts.h"
#include "bootstrap/ui/UsageText.h"

namespace msg5::bootstrap::commands {
    int RunCreateDatabase(int argc, char** argv) {
        auto ro = sources::ResolveFor(argc, const_cast<const char* const*>(argv),
            spec::MakeSpec_CreateDatabase());
        const std::string bdsn = ro.values.count("bootstrap_dsn") ? ro.values.at("bootstrap_dsn") : "";
        const std::string dbname = ro.values.count("dbname") ? ro.values.at("dbname") : "MSG5";
        const std::string owner = ro.values.count("owner") ? ro.values.at("owner") : "msg5_app_owner";
        std::string owner_pass = ro.values.count("owner_pass") ? ro.values.at("owner_pass") : "";
        const std::string encoding = ro.values.count("encoding") ? ro.values.at("encoding") : "UTF8";
        const std::string templ = ro.values.count("template") ? ro.values.at("template") : "template1";
        const bool ask_owner = ro.values.count("ask_owner_pass") && ro.values.at("ask_owner_pass") == "true";
        const bool dry_run = ro.values.count("dry_run") && ro.values.at("dry_run") == "true";
        const bool force = ro.values.count("force") && ro.values.at("force") == "true";
        const bool yes = ro.values.count("yes") && ro.values.at("yes") == "true";

        if (bdsn.empty()) { std::cerr << "[create-database] --bootstrap-dsn is required.\n"; bootstrap::ui::print_usage(); return 2; }
        if (ask_owner && owner_pass.empty()) owner_pass = bootstrap::ui::prompt_hidden("Owner password");

        try {
            PgExecutor sys{ bdsn };
            std::cout << "[create-database] Connected to server.\n";
            // роль
            {
                std::string sql = "select 1 from pg_roles where rolname = " + msg5::sql::quote_lit(owner) + " limit 1";
                bool role_exists = !sys.scalar(sql).empty();
                std::cout << "[plan] role " << owner << (role_exists ? " exists" : " will be created") << "\n";
                if (!dry_run && !role_exists) {
                    if (!force) { std::cout << "[create-database] not forced; nothing executed.\n"; return 0; }
                    if (!yes && !bootstrap::ui::msg5_prompt_yes_no("Proceed creating role?")) { std::cout << "[create-database] canceled.\n"; return 0; }
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
            // база
            {
                bool db_exists = msg5::dbprobe::database_exists(sys, dbname);
                std::cout << "[plan] database " << dbname << (db_exists ? " exists" : " will be created") << "\n";
                if (dry_run) { std::cout << "[dry-run] add --force to execute" << (yes ? "" : " (and --yes to auto-confirm)") << ".\n"; return 0; }
                if (db_exists) { std::cout << "[create-database] database already exists, nothing to do.\n"; return 0; }
                if (!force) { std::cout << "[create-database] not forced; nothing executed.\n"; return 0; }
                if (!yes && !bootstrap::ui::msg5_prompt_yes_no("Proceed creating database?")) { std::cout << "[create-database] canceled.\n"; return 0; }
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
            std::cerr << "[create-database] ERROR: " << ex.what() << "\n"; return 1;
        }
    }
}
