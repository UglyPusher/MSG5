#pragma once
#include <filesystem>
#include <string>
#include "msg5/config/source/SourceBase.h"

namespace msg5::config {

    struct CommandSpec; // forward decl, реализация в .cpp подключит нужный заголовок

    class FileSource final : public SourceBase {
    public:
        // Базовый «тупой» источник: путь задаётся снаружи и неизменяем.
        explicit FileSource(std::filesystem::path path);

        // Ничего не готовим: источник просто читает один JSON без магии.
        void prepare(const CommandSpec& /*spec*/) override;

    protected:
        // Реальная работа источника (SourceBase::fetch no-throw вызовет это).
        FetchResult fetch_impl(const CommandSpec& spec) override;

    private:
        std::filesystem::path path_;
    };

} // namespace msg5::config
