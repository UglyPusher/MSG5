#include "Validate.h"
#include "bootstrap/spec/ValidateSpec.h"
#include "bootstrap/sources/SourcesChain.h"
#include "PgExecutor.h"
#include "DbProbe.h"
#include "bootstrap/ui/UsageText.h"
namespace msg5::bootstrap::commands {
    int RunValidate(int argc, char** argv) {
        auto ro = sources::ResolveFor(argc, const_cast<const char* const*>(argv),
            spec::MakeSpec_Validate());
        const bool ask_pass = ro.values.count("ask_pass") && ro.values.at("ask_pass") == "true";
        std::string dsn = ro.values.count("dsn") ? ro.values.at("dsn") : "";
        if (dsn.empty()) { std::cerr << "[validate] DSN is required.\n"; bootstrap::ui::print_usage(); return 2; }
        try {
            if (ask_pass) { std::cout << "[validate] --ask-pass enabled.\n"; }
            PgExecutor db{ dsn };
            std::cout << "[validate] Connected OK.\n";
            bool is_empty = msg5::dbprobe::database_empty(db);
            bool meta_present = msg5::dbprobe::meta_schema_present(db);
            bool seed_present = msg5::dbprobe::meta_seed_present(db);
            std::cout << "[validate] database_empty: " << (is_empty ? "true" : "false") << "\n";
            std::cout << "[validate] meta_schema_present: " << (meta_present ? "true" : "false") << "\n";
            std::cout << "[validate] meta_seed_present: " << (seed_present ? "true" : "false") << "\n";
            return 0;
        }
        catch (const std::exception& ex) {
            std::cerr << "[validate] ERROR: " << ex.what() << "\n"; return 1;
        }
    }
}
