#pragma once
#include <filesystem>
#include <vector>

namespace msg5::utils {

// Список файлов в каталоге с заданным расширением (без учета регистра).
// Если ext == nullptr или пустая строка — вернёт все файлы. Результат отсортирован.
std::vector<std::filesystem::path>
list_files_with_extension(const std::filesystem::path& dir, const char* ext);

} // namespace msg5::utils
