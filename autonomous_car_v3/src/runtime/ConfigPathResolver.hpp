#pragma once

#include <filesystem>
#include <string>

namespace autonomous_car::runtime {

std::filesystem::path resolveProjectPath(const std::string &relative_path);

} // namespace autonomous_car::runtime
