#ifndef MSG5_BOOTSTRAP_USAGE_TEXT_H
#define MSG5_BOOTSTRAP_USAGE_TEXT_H

#include <iostream>

namespace msg5::bootstrap::ui {

static inline void print_usage() {
        std::cout <<
            R"(MSG5_Bootstrap - CLI

Usage:
  MSG5_Bootstrap validate
      [--dsn "..."] [--config PATH] [--ask-pass]

  MSG5_Bootstrap create-database
      [--bootstrap-dsn "..."] [--config PATH]
      [--dbname MSG5] [--owner msg5_app_owner] [--owner-pass ***|--ask-owner-pass]
      [--encoding UTF8] [--template template1]
      [--dry-run] [--force] [--yes]

  MSG5_Bootstrap apply-meta-structure
      [--app-dsn "..."] [--baseline-dir PATH] [--config PATH] 
      [--tx-mode per-file|single|none] [--continue-on-error]
      [--dry-run] [--force] [--yes]

  MSG5_Bootstrap apply-meta-data
      [--app-dsn "..."] [--data-dir PATH] [--config PATH]
      [--tx-mode per-file|single|none] [--continue-on-error]
      [--dry-run] [--force] [--yes]

Notes:
  - Parameter precedence: CLI > CONFIG > ENV > Prompt (ENV MSG5_BOOTSTRAP_CONFIG points to config path).
  - Default mode is DRY-RUN and prints a plan ([plan] ). To execute, add --force (or auto-confirm with --yes).
)";
    }

} // namespace msg5::bootstrap::ui
#endif // MSG5_BOOTSTRAP_USAGE_TEXT_H
