#include "nds_stub/NdsShaderCatalog.hpp"

#include <algorithm>
#include <fstream>
#include <map>

#include <nlohmann/json.hpp>

namespace beiklive::nds_stub {
namespace {

std::vector<std::string> fallbackShaderTypes()
{
    return {"dot"};
}

const std::map<std::string, int>& drasticSimpleShaderCodes()
{
    static const std::map<std::string, int> codes {
        {"drastic-linear", 0},
        {"drastic-grayscale", 1},
        {"drastic-nds-color", 2},
        {"drastic-natural-vision", 3},
        {"drastic-nds-color-natural-vision", 4},
        {"drastic-lcd1x", 5},
        {"drastic-lcd1x-nds-color", 6},
        {"drastic-lcd1x-natural-vision", 7},
        {"drastic-lcd1x-nds-color-natural-vision", 8},
        {"drastic-zfast", 9},
        {"drastic-zfast-lcd", 10},
        {"drastic-zfast-lcd-brightness", 11},
        {"drastic-zfast-lcd-nds-color", 12},
        {"drastic-zfast-lcd-natural-vision", 13},
        {"drastic-zfast-lcd-nds-color-natural-vision", 14},
        {"drastic-quilez", 15},
        {"drastic-scanlinesd", 17},
        {"drastic-scanlinesd-color", 18},
        {"drastic-scanlinesd-x", 19},
        {"drastic-scanlinesd-color-x", 20},
        {"drastic-dot-d4", 21},
        {"drastic-dot-hv4", 22},
    };
    return codes;
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

int drasticSimpleShaderCode(const std::string& type)
{
    const auto& codes = drasticSimpleShaderCodes();
    const auto it = codes.find(type);
    return it == codes.end() ? -1 : it->second;
}

bool isDrasticSimpleShaderType(const std::string& type)
{
    return drasticSimpleShaderCode(type) >= 0;
}

} // namespace beiklive::nds_stub
