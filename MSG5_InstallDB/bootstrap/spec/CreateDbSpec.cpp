#include "CreateDbSpec.h"
#include "bootstrap/sources/CliKeys.h"
using namespace msg5::config;
namespace msg5::bootstrap::spec {
    CommandSpec MakeSpec_CreateDatabase() {
        CommandSpec cs{}; cs.name = "create-database";
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
}
