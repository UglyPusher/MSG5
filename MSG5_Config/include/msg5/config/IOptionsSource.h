#pragma once
#include <string>
#include <unordered_map>
#include <memory>



namespace msg5::config {
    struct CommandSpec; // forward-decl
    // Кто мы: CLI / FILE / ENV / STDIN
    enum class SourceKind { Cli, File, Env, Stdin };

    struct FetchResult {
        std::unordered_map<std::string, std::string> kv; // сырые пары ключ=строка
    };

    struct IOptionsSource {
        virtual ~IOptionsSource() = default;
        virtual FetchResult fetch(const CommandSpec& spec) = 0;
        virtual SourceKind  kind() const = 0;  // ← ВАЖНО: добавили
    };

    using IOptionsSourcePtr = std::unique_ptr<IOptionsSource>;
}
