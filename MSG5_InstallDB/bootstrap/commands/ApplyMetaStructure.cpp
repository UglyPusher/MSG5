#include <algorithm>

#include "ApplyMetaStructure.h"
#include "bootstrap/spec/ApplyMetaStructureSpec.h"
#include "bootstrap/sources/SourcesChain.h"
#include "PgExecutor.h"
#include "Utils/FS.h"
#include "bootstrap/UI/Prompts.h"
#include "bootstrap/UI/UsageText.h"
#include "TextUtil.h"

namespace msg5::bootstrap::commands {
    int RunApplyMetaStructure(int argc, char** argv) {
        auto ro = sources::ResolveFor(argc, const_cast<const char* const*>(argv),
            spec::MakeSpec_ApplyMetaStructure());
        const std::string app_dsn = ro.values.count("app_dsn") ? ro.values.at("app_dsn") : "";
        const std::string base_dir = ro.values.count("baseline_dir") ? ro.values.at("baseline_dir") : "";
        const std::string tx_mode = ro.values.count("tx_mode") ? ro.values.at("tx_mode") : "per-file";
        const bool cont_on_err = ro.values.count("continue_on_error") && ro.values.at("continue_on_error") == "true";
        const bool dry_run = ro.values.count("dry_run") && ro.values.at("dry_run") == "true";
        const bool force = ro.values.count("force") && ro.values.at("force") == "true";
        const bool yes = ro.values.count("yes") && ro.values.at("yes") == "true";
        if (app_dsn.empty() || base_dir.empty()) {
            std::cerr << "[apply-meta-structure] required params missing.\n";
            bootstrap::ui::print_usage();
            return 2;
        }
        
        if (!(tx_mode == "per-file" || tx_mode == "single" || tx_mode == "none")) {
            std::cerr << "[apply-meta-structure] invalid --tx-mode: " << tx_mode
                << " (allowed: per-file|single|none)\n";
            return 2;
        }

        std::vector<std::filesystem::path> files;
        try { files = msg5::utils::list_files_with_extension(std::filesystem::path(base_dir), ".sql"); }
        catch (const std::exception& ex) { std::cerr << "[apply-meta-structure] FS error: " << ex.what() << "\n"; return 2; }
        if (files.empty()) { std::cout << "[apply-meta-structure] No .sql files in: " << base_dir << "\n"; return 0; }
        // стабильный и предсказуемый порядок применения
        sort_paths_by_name(files);

        std::cout << "[plan] tx-mode = " << tx_mode << ", files = " << files.size() << (cont_on_err ? ", continue-on-error" : "") << "\n";
        for (auto& f : files) std::cout << "  - " << f.string() << "\n";
        if (dry_run) { std::cout << "[dry-run] add --force to execute" << (yes ? "" : " (and --yes to auto-confirm)") << ".\n"; return 0; }
        if (!force) { std::cout << "[apply-meta-structure] not forced; nothing executed.\n"; return 0; }
        if (!yes && !bootstrap::ui::msg5_prompt_yes_no("Proceed applying meta structure?")) { std::cout << "[apply-meta-structure] canceled.\n"; return 0; }

        PgExecutor db{ app_dsn };
        auto run_file = [&](const std::filesystem::path& p)->bool {
            std::string sql;
            if (!read_file_text(p, sql)) {
                std::cerr << "[apply-meta-structure] cannot open: " << p << "\n";
                return false;
            }
            
            if (is_all_whitespace(sql)) {
                std::cout << "[skip] " << p.filename().string() << " (empty)\n";
                return true;
            }

            try { db.exec(sql); std::cout << "[ok] " << p.filename().string() << "\n"; return true; }
            catch (const std::exception& ex) { std::cerr << "[ERR] " << p.filename().string() << ": " << ex.what() << "\n"; return false; }
            };

        if (tx_mode == "single") {
            try {
                db.exec("begin;"); bool all_ok = true;
                for (auto& f : files) { if (!run_file(f)) { all_ok = false; if (!cont_on_err) break; } }
                if (!all_ok && !cont_on_err) { db.exec("rollback;"); std::cerr << "[apply-meta-structure] aborted, rolled back.\n"; return 1; }
                db.exec("commit;");
            }
            catch (const std::exception& ex) {
                try { db.exec("rollback;"); }
                catch (...) {};
                std::cerr << "[apply-meta-structure] TX error: " << ex.what() << "\n"; return 1;
            }
        }
        else if (tx_mode == "per-file") {
            bool any_err = false;
            for (auto& f : files) {
                try { db.exec("begin;"); bool ok = run_file(f); db.exec(ok ? "commit;" : "rollback;"); if (!ok) { any_err = true; if (!cont_on_err) break; } }
                catch (const std::exception& ex) {
                    try { db.exec("rollback;"); }
                    catch (...) {};
                    any_err = true; std::cerr << "[apply-meta-structure] TX error on file " << f.filename().string() << ": " << ex.what() << "\n"; if (!cont_on_err) break;
                }
            }
            if (any_err && !cont_on_err) return 1;
        }
        else {
            bool any_err = false; for (auto& f : files) { if (!run_file(f)) { any_err = true; if (!cont_on_err) break; } }
            if (any_err && !cont_on_err) return 1;
        }
        std::cout << "[apply-meta-structure] done.\n"; return 0;
    }
}
