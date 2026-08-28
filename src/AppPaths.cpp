#include "AppPaths.h"

#include <array>
#include <cstdlib>
#include <system_error>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace {

std::filesystem::path g_app_dir;
bool g_paths_initialized = false;

std::filesystem::path normalized_absolute_path(const std::filesystem::path &path)
{
    std::error_code error;
    const std::filesystem::path absolute =
        path.is_absolute() ? path : std::filesystem::absolute(path, error);
    if (!error && !absolute.empty()) {
        return absolute.lexically_normal();
    }
    return path.lexically_normal();
}

std::filesystem::path detect_executable_path(const char *argv0)
{
#if defined(__linux__)
    std::array<char, 4096> buffer{};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length > 0) {
        buffer[static_cast<std::size_t>(length)] = '\0';
        return normalized_absolute_path(std::filesystem::path(buffer.data()));
    }
#endif

    if (argv0 != nullptr && argv0[0] != '\0') {
        return normalized_absolute_path(std::filesystem::path(argv0));
    }

    std::error_code error;
    return std::filesystem::current_path(error);
}

std::filesystem::path detect_app_dir(const char *argv0)
{
    const char *override_dir = std::getenv("PROGEN3D_APPDIR");
    if (override_dir != nullptr && override_dir[0] != '\0') {
        return normalized_absolute_path(std::filesystem::path(override_dir));
    }

    const std::filesystem::path executable_path = detect_executable_path(argv0);
    if (executable_path.has_parent_path()) {
        return executable_path.parent_path().lexically_normal();
    }

    std::error_code error;
    return std::filesystem::current_path(error).lexically_normal();
}

}  // namespace

void ProgenyApplicationPathLocator::initialize(const char *argv0)
{
    g_app_dir = detect_app_dir(argv0);
    g_paths_initialized = true;
}

std::filesystem::path ProgenyApplicationPathLocator::applicationDirectory()
{
    if (!g_paths_initialized) {
        initialize(nullptr);
    }
    return g_app_dir;
}

std::filesystem::path ProgenyApplicationPathLocator::resourcePath(const std::filesystem::path &relative_path)
{
    if (relative_path.empty()) {
        return applicationDirectory();
    }
    if (relative_path.is_absolute()) {
        return relative_path.lexically_normal();
    }
    return (applicationDirectory() / relative_path).lexically_normal();
}

std::filesystem::path ProgenyApplicationPathLocator::stateDirectory()
{
    const char *override_dir = std::getenv("PROGEN3D_STATE_DIR");
    if (override_dir != nullptr && override_dir[0] != '\0') {
        return normalized_absolute_path(std::filesystem::path(override_dir));
    }

    const char *xdg_config_home = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config_home != nullptr && xdg_config_home[0] != '\0') {
        return normalized_absolute_path(std::filesystem::path(xdg_config_home) / "progen3d");
    }

    const char *home = std::getenv("HOME");
    if (home != nullptr && home[0] != '\0') {
        return normalized_absolute_path(std::filesystem::path(home) / ".config" / "progen3d");
    }

    return applicationDirectory();
}

std::filesystem::path ProgenyApplicationPathLocator::statePath(const std::string &filename)
{
    return (stateDirectory() / filename).lexically_normal();
}

ApplicationPathLocator &progen3d_path_locator()
{
    static ProgenyApplicationPathLocator path_locator;
    return path_locator;
}

void progen3d_initialize_paths(const char *argv0)
{
    progen3d_path_locator().initialize(argv0);
}

std::filesystem::path progen3d_app_dir()
{
    return progen3d_path_locator().applicationDirectory();
}

std::filesystem::path progen3d_resource_path(const std::filesystem::path &relative_path)
{
    return progen3d_path_locator().resourcePath(relative_path);
}

std::filesystem::path progen3d_state_dir()
{
    return progen3d_path_locator().stateDirectory();
}

std::filesystem::path progen3d_state_path(const std::string &filename)
{
    return progen3d_path_locator().statePath(filename);
}
