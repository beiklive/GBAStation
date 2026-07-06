#pragma once

#include <string>
#include <vector>

namespace beiklive::nds_stub {

const std::vector<std::string>& availableNdsShaderTypes();
bool isKnownNdsShaderType(const std::string& type);
std::string normalizeNdsShaderType(const std::string& type);

} // namespace beiklive::nds_stub
