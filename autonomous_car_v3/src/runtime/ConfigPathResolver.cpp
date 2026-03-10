#include "runtime/ConfigPathResolver.hpp"

#include <system_error>

namespace autonomous_car::runtime {

std::filesystem::path resolveProjectPath(const std::string &relative_path) {
    namespace fs = std::filesystem;

    const fs::path relative(relative_path);
    std::error_code error;
    const fs::path executable_path = fs::read_symlink("/proc/self/exe", error);
    if (!error) {
        const fs::path candidate = executable_path.parent_path().parent_path() / relative;
        if (fs::exists(candidate)) {
            return candidate;
        }
    }

    const fs::path fallback = fs::current_path() / relative;
    if (fs::exists(fallback)) {
        return fallback;
    }

    if (!error) {
        return executable_path.parent_path().parent_path() / relative;
    }

    return fallback;
}

} // namespace autonomous_car::runtime
