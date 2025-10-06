#pragma once
#include "msg5/config/source/SourceBase.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace msg5::config {
	struct CommandSpec;  // forward
	
	class ArgsSource final : public SourceBase {
	public:
		ArgsSource() : SourceBase(ProviderClass::Cli, "cli") {}
		ArgsSource(int argc, const char* const* argv);
		explicit ArgsSource(std::vector<std::string> argv);
		// подготовим кэш сопоставлений флагов командной строки
		void prepare(const CommandSpec & spec) override;
		// Реальная работа источника (SourceBase::fetch no-throw вызовет это)
		FetchResult fetch_impl(const CommandSpec & spec) override;
	
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
