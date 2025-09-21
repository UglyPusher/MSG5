#pragma once
// Normalizing wrappers for prompt_line / prompt_hidden
// They return std::string regardless of the underlying return type.
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "Prompts.h"

namespace msg5::prompt {
    template <class T>
    inline std::string to_string_or(T&& v, std::string_view def = {}) {
        using V = std::decay_t<T>;
        if constexpr (std::is_same_v<V, std::string>) {
            return v.empty() ? std::string(def) : std::move(v);
        } else { // assume std::optional<std::string>
            return v ? *v : std::string(def);
        }
    }

    inline std::string line(std::string_view title, std::string_view def = {}) {
        return to_string_or(prompt_line(title, def), def);
    }

    inline std::string hidden(std::string_view title, std::string_view def = {}) {
        return to_string_or(prompt_hidden(title), def);
    }
} // namespace msg5::prompt
