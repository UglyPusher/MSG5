#pragma once
#include <vector>
#include <memory> // for std::unique_ptr
#include "ResolvedOptions.h"
//#include "source/IOptionsSource.h"
//#include "msg5/config/CommandSpec.h"


namespace msg5::config {
    struct CommandSpec; // объявление вперед

    struct IOptionsSource;
    using IOptionsSourcePtr = std::unique_ptr<IOptionsSource>;

    class Resolver {
    public:
        explicit Resolver(std::vector<IOptionsSourcePtr> sources);
        ResolvedOptions resolve(const CommandSpec& spec);
    private:
        std::vector<IOptionsSourcePtr> sources_;
    };
}
