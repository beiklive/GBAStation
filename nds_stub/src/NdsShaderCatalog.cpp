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
    if (it != codes.end())
        return it->second;

    if (type == "drastic-none" || type == "drastic-linear" || type == "drastic-linear2x")
        return 0;
    if (type.find("sharp-bilinear-nds-color-natural") != std::string::npos)
        return 4;
    if (type.find("sharp-bilinear-nds-color") != std::string::npos || type.find("scale-color-correction") != std::string::npos)
        return 2;
    if (type.find("sharp-bilinear-natural") != std::string::npos)
        return 3;
    if (type.find("sharp-bilinear") != std::string::npos)
        return 0;
    if (type.find("grayscale") != std::string::npos)
        return 1;
    if (type.find("nds-color-natural") != std::string::npos)
        return 4;
    if (type.find("nds-color") != std::string::npos && type.find("lcd") == std::string::npos && type.find("zfast") == std::string::npos)
        return 2;
    if ((type.find("natural-vision") != std::string::npos || type == "drastic-natural") &&
        type.find("lcd") == std::string::npos &&
        type.find("zfast") == std::string::npos)
    {
        return 3;
    }
    if (type.find("lcd1x-nds-color-natural") != std::string::npos || type.find("lcd3x-nds-color-natural") != std::string::npos)
        return 8;
    if (type.find("lcd1x-nds-color") != std::string::npos || type.find("lcd3x-nds-color") != std::string::npos)
        return 6;
    if (type.find("lcd1x-natural") != std::string::npos || type.find("lcd3x-natural") != std::string::npos)
        return 7;
    if (type.find("lcd1x") != std::string::npos || type.find("lcd3x") != std::string::npos)
        return 5;
    if (type.find("zfast-lcd-nds-color-natural") != std::string::npos)
        return 14;
    if (type.find("zfast-lcd-nds-color") != std::string::npos)
        return 12;
    if (type.find("zfast-lcd-natural") != std::string::npos)
        return 13;
    if (type.find("zfast-lcd-brightness") != std::string::npos)
        return 11;
    if (type.find("zfast-lcd") != std::string::npos)
        return 10;
    if (type.find("zfast") != std::string::npos)
        return 9;
    if (type.find("quilez") != std::string::npos)
        return 15;
    if (type.find("scanlinesd-color-x") != std::string::npos || type.find("scanlinesdcolorx") != std::string::npos)
        return 20;
    if (type.find("scanlinesd-x") != std::string::npos || type.find("scanlinesdx") != std::string::npos)
        return 19;
    if (type.find("scanlinesd-color") != std::string::npos || type.find("scanlinesdcolor") != std::string::npos)
        return 18;
    if (type.find("scanlinesd") != std::string::npos || type.find("scanline") != std::string::npos || type.find("crt") != std::string::npos)
        return 17;
    if (type.find("dot-d4") != std::string::npos)
        return 21;
    if (type.find("dot-hv4") != std::string::npos)
        return 22;
    if (type.find("dot") != std::string::npos)
        return 21;
    if (type.find("fxaa") != std::string::npos ||
        type.find("smaa") != std::string::npos ||
        type.find("aa") != std::string::npos ||
        type.find("cartoon") != std::string::npos ||
        type.find("bloom") != std::string::npos ||
        type.find("luna") != std::string::npos ||
        type.find("nataa") != std::string::npos)
    {
        return 15;
    }
    return -1;
}

bool isDrasticSimpleShaderType(const std::string& type)
{
    return drasticSimpleShaderCode(type) >= 0;
}

} // namespace beiklive::nds_stub
