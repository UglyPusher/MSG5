#pragma once
#include "msg5/config/source/SourceBase.h"

namespace msg5::config {
    struct CommandSpec;  // forward
    class PromptSource final : public SourceBase {
    public:
        // interactive=true — построчный ввод при запуске из консоли (TTY)
        explicit PromptSource(bool interactive = false)
            : SourceBase(ProviderClass::Stdin, "stdin"), interactive_(interactive) {}

        
        // stdin не требует подготовки, но оставим метод для единообразия
        void prepare(const CommandSpec& /*spec*/) override {}

    protected:
        FetchResult fetch_impl(const CommandSpec & spec) override;

    private:
        bool interactive_{ false };
    };

;
}
