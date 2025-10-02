#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include "msg5/config/source/IOptionsSource.h"  // IOptionsSource, IOptionsSourcePtr
#include "msg5/config/OptionsSourceTypes.h"     // ProviderClass (если нужно где-то снаружи)

namespace msg5::config {

	// CLI
	IOptionsSourcePtr makeArgsSource(int argc, const char* const* argv);
	IOptionsSourcePtr makeArgsSource(std::vector<std::string> argv);

	// FILE
	IOptionsSourcePtr makeFileSource(std::filesystem::path path);

	// ENV (префикс задаёт приложение, например "MSG5_")
	IOptionsSourcePtr makeEnvSource(std::string prefix);

	// STDIN
	IOptionsSourcePtr makePromptSource();

} // namespace msg5::config
