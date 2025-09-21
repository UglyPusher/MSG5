#pragma once
#include "SourceBase.h"
#include <vector>
#include <string>

namespace msg5::config {
    struct ArgsSource : SourceBase {
        ArgsSource() : SourceBase(SourceKind::Cli) {}
        ArgsSource(int argc, const char* const* argv);
        explicit ArgsSource(std::vector<std::string> argv);

        FetchResult fetch(const CommandSpec& spec) override;

    private:
        std::vector<std::string> argv_;
    };
}
