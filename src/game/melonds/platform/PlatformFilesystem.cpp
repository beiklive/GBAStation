#include "platform/PlatformFilesystem.hpp"

#include <borealis.hpp>

namespace beiklive::melonds::platform
{

FILE* openFile(const std::string& path, const std::string& mode)
{
    return fopen(path.c_str(), mode.c_str());
}

FILE* openLocalFile(const std::string& path, const std::string& mode)
{
    std::string fullPath = getConfigPath() + path;
    FILE* f = fopen(fullPath.c_str(), mode.c_str());
    if (!f)
        f = fopen(path.c_str(), mode.c_str());
    return f;
}

void closeFile(FILE* f)
{
    if (f) fclose(f);
}

bool fileExists(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool localFileExists(const std::string& path)
{
    FILE* f = openLocalFile(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

std::string getConfigPath()
{
#ifdef __SWITCH__
    return "sdmc:/switch/GBAStation/";
#elif defined(_WIN32)
    return ".\\";
#else
    const char* home = getenv("HOME");
    if (home)
        return std::string(home) + "/.config/GBAStation/";
    return "./";
#endif
}

std::string getBiosPath()
{
    return getConfigPath();
}

} // namespace beiklive::melonds::platform
