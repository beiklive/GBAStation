#include "Utils/strUtils.hpp"

namespace beiklive::string
{


std::string getFileSuffix(std::string filename) {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos || pos == filename.length() - 1) {
        return "";
    }
    return filename.substr(pos + 1);
}


bool iequals(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](char ca, char cb) {
                          return std::tolower(static_cast<unsigned char>(ca)) ==
                                 std::tolower(static_cast<unsigned char>(cb));
                      });
}


bool isPathString(const std::string& str) {
    return str.find('/') != std::string::npos ||
           str.find('\\') != std::string::npos;
}


std::string extractFileName(const std::string& path) {
    // 查找最后一个 '/' 或 '\'
    size_t last_slash = path.find_last_of('/');
    size_t last_backslash = path.find_last_of('\\');
    size_t pos = std::string::npos;

    if (last_slash != std::string::npos && last_backslash != std::string::npos) {
        pos = (last_slash > last_backslash) ? last_slash : last_backslash;
    } else if (last_slash != std::string::npos) {
        pos = last_slash;
    } else if (last_backslash != std::string::npos) {
        pos = last_backslash;
    }

    // 没有分隔符 → 整个字符串就是文件名
    if (pos == std::string::npos) {
        return path;
    }

    // 分隔符在末尾 → 返回空
    if (pos == path.length() - 1) {
        return "";
    }

    // 返回分隔符之后的部分
    return path.substr(pos + 1);
}


} // namespace beiklive
