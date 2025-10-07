#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include "msg5/config/source/SourceBase.h"
#include "msg5/config/source/IOptionsSource.h"  // IOptionsSource, IOptionsSourcePtr
#include "msg5/config/OptionsSourceTypes.h"     // ProviderClass (если нужно где-то снаружи)

namespace msg5::config {

	// CLI
	[[nodiscard]] IOptionsSourcePtr makeArgsSource(int argc, const char* const* argv,
		LogLevel min = LogLevel::Trace) noexcept;

	[[nodiscard]] IOptionsSourcePtr makeEnvSource(std::string_view prefix,
		LogLevel min = LogLevel::Trace) noexcept;

	[[nodiscard]] IOptionsSourcePtr makeFileSource(std::filesystem::path p,
		LogLevel min = LogLevel::Trace) noexcept;

	[[nodiscard]] IOptionsSourcePtr makePromptSource(bool interactive = false,
		LogLevel min = LogLevel::Trace) noexcept;

} // namespace msg5::config
