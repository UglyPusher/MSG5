#include "ApplyMetaStructureSpec.h"
#include "bootstrap/sources/CliKeys.h"
#include "SpecCommon.h"

using namespace msg5::config;
namespace msg5::bootstrap::spec {
    CommandSpec MakeSpec_ApplyMetaStructure() {
        CommandSpec cs{}; cs.name = "apply-meta-structure";
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
}
