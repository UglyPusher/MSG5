#include "msg5/config/Resolver.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/source/IOptionsSource.h" // prepare(), fetch(), kind()
#include "msg5/config/CommandSpec.h"
#include <iostream>
#include <unordered_set>


// #include "Validators.h" // позже
// #include "DefaultsFiller.h" // позже

using namespace msg5::config;


// Оставляем в спецификации только те опции, которых ещё нет в результирующем наборе
static CommandSpec filter_missing(const CommandSpec & spec,
    const std::unordered_map<std::string, std::string>&have) noexcept {
    CommandSpec filtered;
    filtered.options.reserve(spec.options.size());
    for (const auto& opt : spec.options) {
        if (have.find(opt.key) == have.end()) {
            filtered.options.push_back(opt);
        }
    }
    return filtered;
}

void Resolver::emit(LogLevel level, std::string_view code, std::string_view msg) const {
    // свои события резолвера — через общий logger_
    logger_.emit(level, code, msg);
    // и сразу отдаём подписчикам (logger_ уже отфильтровал по min)
    LogEvent ev{ level, "resolver", std::string(code), std::string(msg) };
    for (const auto& s : sinks_) s(ev);
}

Resolver::Resolver(std::vector<IOptionsSourcePtr> sources) noexcept
    : sources_(std::move(sources)) {
    // Мост: события всех источников → подписчики резолвера (с единым min)
    for (auto& sp : sources_) {
        sp->subscribe([this](const LogEvent& ev) {
            if (log_rank(ev.level) > log_rank(logger_.min())) return;
            for (const auto& s : sinks_) s(ev);
            });
    }
}

ResolvedOptions Resolver::resolve(const CommandSpec& spec) noexcept {
    ResolvedOptions out;
    out.values.reserve(spec.options.size());
    out.value_sources.reserve(spec.options.size());

    // 0) Подготовка источников под текущую спецификацию (кэш маппингов и т.п.)
    for (auto& s : sources_) {
        s->prepare(spec);
    }
    emit(LogLevel::Debug, "PREPARED", "all sources");

    // 1) Слить источники по приоритету: ранний источник сильнее — поздние НЕ перезаписывают
    for (auto& s : sources_) {
        // отдаём источнику только «дыры», чтобы он не тратил усилия
        const CommandSpec need = filter_missing(spec, out.values);
        emit(LogLevel::Debug, "ASK",
            std::string(to_string(s->kind())) + " [" + s->id() + "]" +
            " need=" + std::to_string(need.options.size()) +
            " already=" + std::to_string(out.values.size()));

        if (need.options.empty()) {
            emit(LogLevel::Debug, "SKIP", std::string(to_string(s->kind())) + " [" + s->id() + "] no holes");
            continue; // уже всё закрыли более ранними слоями
        }

        auto r = s->fetch(need);
        emit(LogLevel::Info, "GOT",
            std::string(to_string(s->kind())) + " [" + s->id() + "]" +
            " kv=" + std::to_string(r.kv.size()) +
            " src=" + to_string(r.source));

        const ValueSource src_origin = r.source; // источник приходит от провайдера


        // Быстрый фильтр "ожидали ли вообще такой ключ"
        std::unordered_set<std::string> expected;
        expected.reserve(need.options.size());
        for (const auto& o : need.options) expected.insert(o.key);

        for (const auto& [k, v] : r.kv) {
            if (expected.find(k) == expected.end()) {
                emit(LogLevel::Debug, "GOT_UNKNOWN_KEY", s->id() + " returned extra key: " + k);
                continue;
            }

            // «Ранний сильнее»: если ключ уже установлен ранее — не трогаем
            if (out.values.find(k) == out.values.end()) {
                out.values.emplace(k, v);
                out.value_sources.emplace(k, src_origin);
                //out.value_sources.emplace(k, r.source);
            }
            // иначе оставляем предыдущее значение и его origin
        }
        
        // Ранний выход: если закрыли все опции, дальше нет смысла опрашивать
        if (out.values.size() >= spec.options.size()) {
            emit(LogLevel::Info, "DONE", "all " + std::to_string(out.values.size()) + " collected");
            break;
        }
    }

    if (out.values.size() < spec.options.size()) {
        emit(LogLevel::Info, "SUMMARY", "collected=" + std::to_string(out.values.size()) + " of " + std::to_string(spec.options.size()));
    }

    // DefaultsFiller::apply(spec, out); // позже
    // validate(spec, out); // позже

    return out;
}
