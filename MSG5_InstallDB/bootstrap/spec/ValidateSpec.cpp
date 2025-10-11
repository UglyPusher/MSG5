#include "ValidateSpec.h"
#include "bootstrap/sources/CliKeys.h"
using namespace msg5::config;
namespace msg5::bootstrap::spec {
    CommandSpec MakeSpec_Validate() {
        CommandSpec cs{}; cs.name = "validate";
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
}
