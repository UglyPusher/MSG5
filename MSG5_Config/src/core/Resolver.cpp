#include "msg5/config/Resolver.h"
#include "msg5/config/errors.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/source/IOptionsSource.h" // prepare(), fetch(), kind()
#include "msg5/config/CommandSpec.h"
// #include "Validators.h" // позже
// #include "DefaultsFiller.h" // позже

using namespace msg5::config;

Resolver::Resolver(std::vector<IOptionsSourcePtr> sources)
    : sources_(std::move(sources)) {
}

ResolvedOptions Resolver::resolve(const CommandSpec& spec) {
    ResolvedOptions out;

    // Слить источники по приоритету: ранний источник сильнее — перезаписывает
    /*for (auto& s : sources_) {
        auto r = s->fetch(spec);
        for (auto& [k, v] : r.kv) {
            // Пишем всегда, ранний источник перетрет последующие
            out.values[k] = v;
            out.origins[k] = Origin::Cli; // ПЛЕЙСХОЛДЕР: позже поставим реальный origin в провайдерах
        }
    }*/

    // 0) Подготовка источников под текущую спецификацию (кэш маппингов и т.п.)
    for (auto& s : sources_) {
        s->prepare(spec);
    }

    // Хелпер: маппинг SourceKind → Origin
    auto map_kind = [](SourceKind k) -> Origin {
        switch (k) {
        case SourceKind::Cli:   return Origin::Cli;
        case SourceKind::File:  return Origin::ConfigFile;
        case SourceKind::Env:   return Origin::Env;
        case SourceKind::Stdin: return Origin::Stdin;
        }
        // на всякий случай
        return Origin::Default;
    };

    // 1) Слить источники по приоритету: ранний источник сильнее — перезаписывает
    for (auto& s : sources_) {
        const Origin src_origin = map_kind(s->kind());
        const auto r = s->fetch(spec); // no-throw по контракту
        for (const auto& [k, v] : r.kv) {
            // «Ранний сильнее»: если ключ уже установлен ранее — не трогаем
            if (out.values.find(k) == out.values.end()) {
                out.values.emplace(k, v);
                out.origins.emplace(k, src_origin);
            }
            // иначе оставляем предыдущее значение и его origin
        }
    }

    // DefaultsFiller::apply(spec, out); // позже
    // validate(spec, out); // позже

    return out;
}
