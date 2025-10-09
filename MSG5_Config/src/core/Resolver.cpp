#include "msg5/config/Resolver.h"
#include "msg5/config/ResolvedOptions.h"
#include "msg5/config/CommandSpec.h"
#include "msg5/config/source/IOptionsSource.h"

#include <unordered_set>
#include <string>
#include <utility>
#include <unordered_map>

using namespace msg5::config;

static CommandSpec filter_missing(const CommandSpec& spec,
    const std::unordered_map<std::string, std::string>& have) noexcept {
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
    logger_.emit(level, code, msg);
    LogEvent ev{ level, "resolver", std::string(code), std::string(msg) };
    for (const auto& s : sinks_) s(ev);
}

Resolver::Resolver(std::vector<IOptionsSourcePtr> sources) noexcept
    : sources_(std::move(sources)) {

    // Прокидываем логи источников подписчикам резолвера (с учётом нашего min level)
    for (auto& sp : sources_) {
        sp->subscribe([this](const LogEvent& ev) {
            if (log_rank(ev.level) > log_rank(logger_.min())) return;
            for (const auto& s : sinks_) s(ev);
            });
    }
}

ResolvedOptions Resolver::resolve(const CommandSpec& spec) {
    return resolve(spec, ResolveParams{});
}

ResolvedOptions Resolver::resolve(const CommandSpec& spec, const ResolveParams& p) {
    ResolvedOptions ro = collect_(spec);
    maybe_apply_defaults_(ro, spec, p);
    maybe_validate_and_log_(ro, spec, p);
    return ro;
}

ResolvedOptions Resolver::collect_(const CommandSpec& spec) {
    ResolvedOptions out;
    out.values.reserve(spec.options.size());
    out.value_sources.reserve(spec.options.size());

    // 0) подготовка источников
    for (auto& s : sources_) {
        s->prepare(spec);
    }
    emit(LogLevel::Debug, "PREPARED", "all sources");

    // 1) опрос источников по приоритету
    for (auto& s : sources_) {
        const CommandSpec need = filter_missing(spec, out.values);

        std::string ask_msg = "id=" + s->id()
            +" need=" + std::to_string(need.options.size())
            +" already=" + std::to_string(out.values.size());
        emit(LogLevel::Debug, "ASK", ask_msg);

        if (need.options.empty()) {
            emit(LogLevel::Debug, "SKIP", "id=" + s->id() + " no holes");
            continue;
        }

        auto r = s->fetch(need);
        emit(LogLevel::Info, "GOT",
            "id=" + s->id() + " kv=" + std::to_string(r.kv.size()));

        std::unordered_set<std::string> expected;
        expected.reserve(need.options.size());
        for (const auto& o : need.options) expected.insert(o.key);

        for (const auto& [k, v] : r.kv) {
            if (expected.find(k) == expected.end()) {
                emit(LogLevel::Debug, "GOT_UNKNOWN_KEY", "id=" + s->id() + " key=" + k);
                continue;
            }
            if (out.values.find(k) == out.values.end()) {
                out.values.emplace(k, v);
                out.value_sources.emplace(k, r.source); // кто положил это значение
            }
        }

        if (out.values.size() >= spec.options.size()) {
            emit(LogLevel::Info, "DONE",
                "all " + std::to_string(out.values.size()) + " collected");
            break;
        }
    }

    if (out.values.size() < spec.options.size()) {
        emit(LogLevel::Info, "SUMMARY",
            "collected=" + std::to_string(out.values.size())
            + " of " + std::to_string(spec.options.size()));
    }

    return out;
}

void Resolver::maybe_apply_defaults_(ResolvedOptions& ro,
    const CommandSpec& spec,
    const ResolveParams& p) {
    if (!p.apply_defaults) return;
    const auto n = ro.apply_defaults(spec);
    if (n) emit(LogLevel::Info, "DEFAULTS_APPLIED", "count=" + std::to_string(n));
}

void Resolver::maybe_validate_and_log_(const ResolvedOptions& ro,
    const CommandSpec& spec,
    const ResolveParams& p) {
    if (!p.validate) return;

    ResolvedOptions::ValidationResult local;
    auto* out = p.out_errors ? p.out_errors : &local;
    out->clear();

    const bool ok = ro.validate(spec, *out);

    if (!p.log_validation) return;

    if (!ok) {
        for (const auto& e : *out)
            emit(LogLevel::Warn, "VALIDATION_ERROR", e.key + ": " + e.message);
        emit(LogLevel::Warn, "VALIDATION_SUMMARY",
            "errors=" + std::to_string(out->size()));
    }
    else {
        emit(LogLevel::Info, "VALIDATION_OK", "all checks passed");
    }
}
