#pragma once
#include <vector>
#include "ResolvedOptions.h"
#include "IOptionsSource.h"
#include "msg5/config/CommandSpec.h"

//struct CommandSpec; // объявление вперед

namespace msg5::config {
    class Resolver {
    public:
        explicit Resolver(std::vector<IOptionsSourcePtr> sources);
        ResolvedOptions resolve(const CommandSpec& spec);
    private:
        std::vector<IOptionsSourcePtr> sources_;
    };
}
