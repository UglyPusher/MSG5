#pragma once
#include "msg5/config/source/SourceBase.h"

namespace msg5::config {
    struct CommandSpec;  // forward
    class PromptSource final : public SourceBase {
    public:
        // interactive=true — построчный ввод при запуске из консоли (TTY)
        explicit PromptSource(bool interactive, LogLevel min) noexcept
            : SourceBase(ProviderClass::Stdin, "stdin")
            , interactive_(interactive)
        {
            set_min_level(min);
        }


        // stdin не требует подготовки, но оставим метод для единообразия
        void prepare(const CommandSpec& /*spec*/) noexcept override {}

    protected:
        FetchResult fetch_impl(const CommandSpec& spec) noexcept override;

    private:
        bool interactive_{ false };
    };
}
