#pragma once
#include "msg5/config/IOptionsSource.h"
#include "msg5/config/CommandSpec.h"

namespace msg5::config {
    class SourceBase : public IOptionsSource {
    public:
        explicit SourceBase(SourceKind k) noexcept : kind_(k) {}
        SourceKind kind() const noexcept override { return kind_; }
    private:
        SourceKind kind_;
    };
}
