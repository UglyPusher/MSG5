#pragma once
#include "SourceBase.h"

namespace msg5::config {
    struct PromptSource : SourceBase {
        PromptSource() : SourceBase(SourceKind::Stdin) {}
        FetchResult fetch(const CommandSpec& spec) override;
    };
}
