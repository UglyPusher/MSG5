#pragma once
#include "SourceBase.h"

namespace msg5::config {
    struct EnvSource : SourceBase {
        EnvSource() : SourceBase(SourceKind::Env) {}
        FetchResult fetch(const CommandSpec& spec) override;
    };
}
