#pragma once
#include "AdminConfig.h"
#include <iosfwd>

namespace msg5::admin {

    // Коды под автоматизацию (как обсуждали): 0=OK, 7=connect error, 78=config/state
    enum class PreflightResult : int { Ok = 0, ConnectError = 7, ConfigError = 78 };

    // Быстрая проверка окружения и БД.
    // Ничего не делает «по сети», кроме простых пингов и проверок структуры.
    PreflightResult Preflight(const config::AdminConfig& cfg,
        std::ostream& log, std::ostream& err);

} // namespace msg5::admin
