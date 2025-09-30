#pragma once
#include "msg5/config/source/SourceBase.h"

namespace msg5::config {
    struct CommandSpec;  // forward
    class PromptSource final : public SourceBase {
    public:
        PromptSource() : SourceBase(SourceKind::Stdin, "stdin") {}
        
        // stdin не требует подготовки, но оставим метод для единообразия
        void prepare(const CommandSpec& /*spec*/) override {}

    protected:
        FetchResult fetch_impl(const CommandSpec & spec) override;
    };
}
