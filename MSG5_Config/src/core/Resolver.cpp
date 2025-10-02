#include "msg5/config/Resolver.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/source/IOptionsSource.h" // prepare(), fetch(), kind()
#include "msg5/config/CommandSpec.h"
#include <iostream>

// #include "Validators.h" // позже
// #include "DefaultsFiller.h" // позже

using namespace msg5::config;

// Оставляем в спецификации только те опции, которых ещё нет в результирующем наборе
static CommandSpec filter_missing(const CommandSpec & spec,
    const std::unordered_map<std::string, std::string>&have) {
    CommandSpec filtered;
    filtered.options.reserve(spec.options.size());
    for (const auto& opt : spec.options) {
        if (have.find(opt.key) == have.end()) {
            filtered.options.push_back(opt);
        }
    }
    return filtered;
}

Resolver::Resolver(std::vector<IOptionsSourcePtr> sources)
    : sources_(std::move(sources)) {
}

ResolvedOptions Resolver::resolve(const CommandSpec& spec) {
    ResolvedOptions out;

    // 0) Подготовка источников под текущую спецификацию (кэш маппингов и т.п.)
    for (auto& s : sources_) {
        s->prepare(spec);
    }

    // 1) Слить источники по приоритету: ранний источник сильнее — поздние НЕ перезаписывают
    for (auto& s : sources_) {
        // отдаём источнику только «дыры», чтобы он не тратил усилия
        const CommandSpec need = filter_missing(spec, out.values);
        std::cout << "[resolver] ask " << to_string(s->kind())
            << " need=" << need.options.size()
            << " already=" << out.values.size() << "\n";
        if (need.options.empty()) {
            std::cout << "[resolver] skip " << to_string(s->kind()) << " (no holes)\n";
            continue; // уже всё закрыли более ранними слоями
            
        }
        auto r = s->fetch(need);
        std::cout << "[resolver] got from " << to_string(s->kind())
            << " kv=" << r.kv.size()
            << " src=" << to_string(r.source) << "\n";


        const ValueSource src_origin = r.source; // источник приходит от провайдера

        for (const auto& [k, v] : r.kv) {
            // «Ранний сильнее»: если ключ уже установлен ранее — не трогаем
            if (out.values.find(k) == out.values.end()) {
                out.values.emplace(k, v);
                //out.value_sources.emplace(k, src_origin);
                out.value_sources.emplace(k, r.source);
            }
            // иначе оставляем предыдущее значение и его origin
        }
        
        // Ранний выход: если закрыли все опции, дальше нет смысла опрашивать
        if (out.values.size() >= spec.options.size()) {
            std::cout << "[resolver] done (all " << out.values.size() << " collected)\n";
            break;
        }
    }

    // DefaultsFiller::apply(spec, out); // позже
    // validate(spec, out); // позже

    return out;
}
