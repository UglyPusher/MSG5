#pragma once
#include "msg5/config/source/SourceBase.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <string_view>

namespace msg5::config {
	struct CommandSpec;  // forward
	
	class ArgsSource final : public SourceBase {
	public:
		//ArgsSource() : SourceBase(ProviderClass::Cli, "cli") {}
		explicit ArgsSource(int argc, const char* const* argv, LogLevel min = LogLevel::Trace) noexcept;
		explicit ArgsSource(std::vector<std::string> argv, LogLevel min = LogLevel::Trace) noexcept;
		// подготовим кэш сопоставлений флагов командной строки
		void prepare(const CommandSpec & spec) noexcept override;
		// Реальная работа источника (SourceBase::fetch no-throw вызовет это)
		FetchResult fetch_impl(const CommandSpec & spec) noexcept override;
	
	private:
		std::vector<std::string> argv_;
		// "--db-host" / "-H"  -> "db.host"
		std::unordered_map<std::string, std::string> flag_to_key_;
		// "db.host" -> flags (OptSecret и др.)
		std::unordered_map<std::string, unsigned>     key_flags_;

		static std::string normalize_flag(std::string_view f) {
			size_t i = 0;
			while (i < f.size() && (f[i] == '-' || f[i] == '/')) ++i;
			return std::string(f.substr(i));
		}
	};
}
