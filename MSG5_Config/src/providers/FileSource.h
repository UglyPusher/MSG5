#pragma once
#include "SourceBase.h"

namespace msg5::config {
    struct FileSource : SourceBase {
        FileSource() : SourceBase(SourceKind::File) {}
        FetchResult fetch(const CommandSpec& spec) override;
    };
}
