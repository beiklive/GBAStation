#pragma once

#include <string>
#include <vector>

namespace beiklive::nds_stub {

struct NdsShaderListEntry {
    enum class Kind {
        Directory,
        Shader,
    };

    Kind kind = Kind::Shader;
    std::string label;
    std::string shaderType;
    std::vector<std::string> path;
};

const std::vector<std::string>& availableNdsShaderTypes();
bool isKnownNdsShaderType(const std::string& type);
std::string normalizeNdsShaderType(const std::string& type);
std::string ndsShaderDisplayName(const std::string& type);
std::string ndsShaderMatchKey(const std::string& type);
std::vector<NdsShaderListEntry> ndsShaderListEntries(const std::vector<std::string>& path);
std::vector<std::string> ndsShaderListPathForType(const std::string& type);
int drasticSimpleShaderCode(const std::string& type);
bool isDrasticSimpleShaderType(const std::string& type);

} // namespace beiklive::nds_stub
