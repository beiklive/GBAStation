#pragma once

#include <string>
#include <vector>

namespace beiklive::mgba_stub {

struct MgbaShaderListEntry {
    enum class Kind {
        Directory,
        Shader,
    };

    Kind kind = Kind::Shader;
    std::string label;
    std::string shaderType;
    std::vector<std::string> path;
};

const std::vector<std::string>& availableMgbaShaderTypes();
bool isKnownMgbaShaderType(const std::string& type);
std::string normalizeMgbaShaderType(const std::string& type);
std::string MgbaShaderDisplayName(const std::string& type);
std::string MgbaShaderMatchKey(const std::string& type);
std::vector<MgbaShaderListEntry> MgbaShaderListEntries(const std::vector<std::string>& path);
std::vector<std::string> MgbaShaderListPathForType(const std::string& type);

} // namespace beiklive::mgba_stub
