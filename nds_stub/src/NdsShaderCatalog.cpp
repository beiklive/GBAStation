#include "nds_stub/NdsShaderCatalog.hpp"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

namespace beiklive::nds_stub {
namespace {

std::vector<std::string> fallbackShaderTypes()
{
    return {"dot"};
}

std::vector<std::string> loadShaderTypesFromFile(const char* path)
{
    std::ifstream in(path);
    if (!in)
        return {};

    auto parsed = nlohmann::json::parse(in, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_array())
        return {};

    std::vector<std::string> result;
    for (const auto& item : parsed)
    {
        if (!item.is_string())
            continue;
        const std::string type = item.get<std::string>();
        if (type.empty() ||
            std::find(result.begin(), result.end(), type) != result.end())
        {
            continue;
        }
        result.push_back(type);
    }
    return result;
}

std::vector<std::string> loadShaderTypes()
{
    static constexpr const char* paths[] = {
        "romfs:/config/nds_shaders.json",
        "sdmc:/GBAStation/resources/config/nds_shaders.json",
        "/GBAStation/resources/config/nds_shaders.json",
        "resources/config/nds_shaders.json",
    };

    for (const char* path : paths)
    {
        auto loaded = loadShaderTypesFromFile(path);
        if (!loaded.empty())
        {
            if (std::find(loaded.begin(), loaded.end(), "dot") == loaded.end())
                loaded.insert(loaded.begin(), "dot");
            return loaded;
        }
    }

    return fallbackShaderTypes();
}

} // namespace

const std::vector<std::string>& availableNdsShaderTypes()
{
    static const std::vector<std::string> types = loadShaderTypes();
    return types;
}

bool isKnownNdsShaderType(const std::string& type)
{
    const auto& types = availableNdsShaderTypes();
    return std::find(types.begin(), types.end(), type) != types.end();
}

std::string normalizeNdsShaderType(const std::string& type)
{
    return isKnownNdsShaderType(type) ? type : "dot";
}

} // namespace beiklive::nds_stub
