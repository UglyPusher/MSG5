#include "msg5/config/Resolver.h"
#include "msg5/config/errors.h"
#include "msg5/config/ResolvedOptions.h"
// #include "Validators.h" // позже
// #include "DefaultsFiller.h" // позже

using namespace msg5::config;

Resolver::Resolver(std::vector<IOptionsSourcePtr> sources)
    : sources_(std::move(sources)) {
}

ResolvedOptions Resolver::resolve(const CommandSpec& spec) {
    ResolvedOptions out;

    // Слить источники по приоритету: ранний источник сильнее — перезаписывает
    for (auto& s : sources_) {
        auto r = s->fetch(spec);
        for (auto& [k, v] : r.kv) {
            // Пишем всегда, ранний источник перетрет последующие
            out.values[k] = v;
            out.origins[k] = Origin::Cli; // ПЛЕЙСХОЛДЕР: позже поставим реальный origin в провайдерах
        }
    }

    // DefaultsFiller::apply(spec, out); // позже
    // validate(spec, out); // позже

    return out;
}
