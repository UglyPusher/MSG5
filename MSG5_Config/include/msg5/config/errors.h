#pragma once
#include <stdexcept>

namespace msg5::config {
	struct user_error : std::runtime_error { using std::runtime_error::runtime_error; };
	struct system_error : std::runtime_error { using std::runtime_error::runtime_error; };

	enum ExitCode { EXIT_OK = 0, EXIT_USER_ERR = 1, EXIT_SYS_ERR = 2, EXIT_UNKNOWN_ERR = 3 };
}
