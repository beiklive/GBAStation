#include "nds_stub/NdsShaderCatalog.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <utility>

#include <nlohmann/json.hpp>

namespace beiklive::nds_stub {
namespace {

constexpr const char* kDefaultShaderType = "RetroArch_dot";

std::vector<std::string> fallbackShaderTypes()
{
    return {kDefaultShaderType};
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trimCopy(std::string value)
{
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) == 0;
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) == 0;
    }).base(), value.end());
    return value;
}

bool startsWith(const std::string& value, const char* prefix)
{
    const std::string p(prefix);
    return value.size() >= p.size() &&
           std::equal(p.begin(), p.end(), value.begin());
}

std::string canonicalStem(std::string value)
{
    value = trimCopy(lowerAscii(std::move(value)));

    std::string out;
    out.reserve(value.size());
    bool lastWasDash = true;
    for (unsigned char c : value)
    {
        if (std::isalnum(c) != 0)
        {
            out.push_back(static_cast<char>(c));
            lastWasDash = false;
            continue;
        }
        if (!lastWasDash)
        {
            out.push_back('-');
            lastWasDash = true;
        }
    }
    while (!out.empty() && out.back() == '-')
        out.pop_back();
    return out;
}

bool isRetroArchShader(const std::string& type)
{
    return startsWith(type, "RetroArch_");
}

bool isDraSticShader(const std::string& type)
{
    return startsWith(type, "DraStic_");
}

std::string oldRetroArchName(const std::string& type)
{
    if (type == "dot")
        return "RetroArch_dot";
    if (type == "dot-clear")
        return "RetroArch_dot-clear";
    if (type == "xbrz-freescale")
        return "RetroArch_xbrz-freescale";
    if (type == "lcd-grid-v2-nds-color")
        return "RetroArch_lcd-grid-v2-nds-color";
    return {};
}

bool isUnsupportedNdsShaderType(const std::string& type)
{
    const std::string key = ndsShaderMatchKey(type);
    if (!startsWith(key, "drastic-"))
        return false;

    return key.find("cartoon") != std::string::npos ||
           key.find("fxaa") != std::string::npos ||
           key.find("smaa") != std::string::npos ||
           key.find("nataa") != std::string::npos ||
           key.find("aacolor") != std::string::npos ||
           key.find("aa2") != std::string::npos ||
           key == "drastic-aa";
}

std::string drasticCategory(const std::string& type)
{
    const std::string key = ndsShaderMatchKey(type);
    if (key.find("xbr") != std::string::npos) return "XBR";
    if (key.find("sabr") != std::string::npos) return "SABR";
    if (key.find("lcd") != std::string::npos) return "LCD";
    if (key.find("crt") != std::string::npos) return "CRT";
    if (key.find("scanline") != std::string::npos) return "Scanline";
    if (key.find("dot") != std::string::npos) return "Dot";
    if (key.find("scale") != std::string::npos ||
        key.find("hq") != std::string::npos ||
        key.find("linear") != std::string::npos ||
        key.find("sharp") != std::string::npos ||
        key.find("quilez") != std::string::npos ||
        key.find("zfast") != std::string::npos)
    {
        return "Scale";
    }
    return "other";
}

void sortShaderEntries(std::vector<NdsShaderListEntry>& entries)
{
    std::sort(entries.begin(), entries.end(), [](const NdsShaderListEntry& lhs, const NdsShaderListEntry& rhs) {
        const std::string a = lowerAscii(lhs.label);
        const std::string b = lowerAscii(rhs.label);
        if (a != b)
            return a < b;
        return lhs.label < rhs.label;
    });
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
        {"drastic-crt", 24},
        {"drastic-crtc", 25},
        {"drastic-crt-geom", 24},
        {"drastic-crt-geom-no-curvature", 24},
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
            isUnsupportedNdsShaderType(type) ||
            (isDraSticShader(type) && drasticCategory(type) == "other") ||
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
            if (std::find(loaded.begin(), loaded.end(), kDefaultShaderType) == loaded.end())
                loaded.insert(loaded.begin(), kDefaultShaderType);
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
    if (isKnownNdsShaderType(type))
        return type;

    const std::string oldRetro = oldRetroArchName(type);
    if (!oldRetro.empty() && isKnownNdsShaderType(oldRetro))
        return oldRetro;

    const std::string key = ndsShaderMatchKey(type);
    if (!key.empty())
    {
        const auto& types = availableNdsShaderTypes();
        for (const auto& candidate : types)
        {
            if (ndsShaderMatchKey(candidate) == key)
                return candidate;
        }
    }

    return kDefaultShaderType;
}

std::string ndsShaderDisplayName(const std::string& type)
{
    return type.empty() ? kDefaultShaderType : type;
}

std::string ndsShaderMatchKey(const std::string& type)
{
    if (isDraSticShader(type))
        return "drastic-" + canonicalStem(type.substr(8));
    if (startsWith(type, "drastic-"))
        return canonicalStem(type);
    if (isRetroArchShader(type))
        return "retroarch-" + canonicalStem(type.substr(10));
    return canonicalStem(type);
}

std::vector<NdsShaderListEntry> ndsShaderListEntries(const std::vector<std::string>& path)
{
    std::vector<NdsShaderListEntry> result;
    const auto& types = availableNdsShaderTypes();

    if (path.empty())
    {
        const bool hasRetro = std::any_of(types.begin(), types.end(), isRetroArchShader);
        const bool hasDrastic = std::any_of(types.begin(), types.end(), isDraSticShader);
        if (hasRetro)
            result.push_back({NdsShaderListEntry::Kind::Directory, "RetroArch", {}, {"RetroArch"}});
        if (hasDrastic)
            result.push_back({NdsShaderListEntry::Kind::Directory, "DraStic", {}, {"DraStic"}});
        sortShaderEntries(result);
        return result;
    }

    if (path.size() == 1 && path[0] == "RetroArch")
    {
        for (const auto& type : types)
        {
            if (isRetroArchShader(type))
                result.push_back({NdsShaderListEntry::Kind::Shader, ndsShaderDisplayName(type), type, path});
        }
        sortShaderEntries(result);
        return result;
    }

    if (path.size() == 1 && path[0] == "DraStic")
    {
        static const char* order[] = {
            "XBR", "SABR", "LCD", "CRT", "Dot", "Scanline", "Scale",
        };
        std::set<std::string> categories;
        for (const auto& type : types)
        {
            if (isDraSticShader(type))
                categories.insert(drasticCategory(type));
        }
        for (const char* category : order)
        {
            if (categories.find(category) != categories.end())
                result.push_back({NdsShaderListEntry::Kind::Directory, category, {}, {"DraStic", category}});
        }
        sortShaderEntries(result);
        return result;
    }

    if (path.size() == 2 && path[0] == "DraStic")
    {
        for (const auto& type : types)
        {
            if (isDraSticShader(type) && drasticCategory(type) == path[1])
                result.push_back({NdsShaderListEntry::Kind::Shader, ndsShaderDisplayName(type), type, path});
        }
        sortShaderEntries(result);
    }
    return result;
}

std::vector<std::string> ndsShaderListPathForType(const std::string& type)
{
    const std::string normalized = normalizeNdsShaderType(type);
    if (isRetroArchShader(normalized))
        return {"RetroArch"};
    if (isDraSticShader(normalized))
        return {"DraStic", drasticCategory(normalized)};
    return {};
}

int drasticSimpleShaderCode(const std::string& type)
{
    const std::string key = ndsShaderMatchKey(type);
    if (!startsWith(key, "drastic-"))
        return -1;

    const auto& codes = drasticSimpleShaderCodes();
    const auto it = codes.find(key);
    if (it != codes.end())
        return it->second;

    if (key == "drastic-none" || key == "drastic-linear" || key == "drastic-linear2x")
        return 0;
    if (key.find("sharp-bilinear-nds-color-natural") != std::string::npos)
        return 4;
    if (key.find("sharp-bilinear-nds-color") != std::string::npos || key.find("scale-color-correction") != std::string::npos)
        return 2;
    if (key.find("sharp-bilinear-natural") != std::string::npos)
        return 3;
    if (key.find("sharp-bilinear") != std::string::npos)
        return 0;
    if (key.find("grayscale") != std::string::npos)
        return 1;
    if (key.find("nds-color-natural") != std::string::npos)
        return 4;
    if (key.find("nds-color") != std::string::npos && key.find("lcd") == std::string::npos && key.find("zfast") == std::string::npos)
        return 2;
    if ((key.find("natural-vision") != std::string::npos || key == "drastic-natural") &&
        key.find("lcd") == std::string::npos &&
        key.find("zfast") == std::string::npos)
    {
        return 3;
    }
    if (key.find("lcd1x-nds-color-natural") != std::string::npos || key.find("lcd3x-nds-color-natural") != std::string::npos)
        return 8;
    if (key.find("lcd1x-nds-color") != std::string::npos || key.find("lcd3x-nds-color") != std::string::npos)
        return 6;
    if (key.find("lcd1x-natural") != std::string::npos || key.find("lcd3x-natural") != std::string::npos)
        return 7;
    if (key.find("lcd1x") != std::string::npos || key.find("lcd3x") != std::string::npos)
        return 5;
    if (key.find("zfast-lcd-nds-color-natural") != std::string::npos)
        return 14;
    if (key.find("zfast-lcd-nds-color") != std::string::npos)
        return 12;
    if (key.find("zfast-lcd-natural") != std::string::npos)
        return 13;
    if (key.find("zfast-lcd-brightness") != std::string::npos)
        return 11;
    if (key.find("zfast-lcd") != std::string::npos)
        return 10;
    if (key.find("zfast") != std::string::npos)
        return 9;
    if (key.find("quilez") != std::string::npos)
        return 15;
    if (key.find("scanlinesd-color-x") != std::string::npos || key.find("scanlinesdcolorx") != std::string::npos)
        return 20;
    if (key.find("scanlinesd-x") != std::string::npos || key.find("scanlinesdx") != std::string::npos)
        return 19;
    if (key.find("scanlinesd-color") != std::string::npos || key.find("scanlinesdcolor") != std::string::npos)
        return 18;
    if (key.find("crt") != std::string::npos)
        return key.find("crtc") != std::string::npos ? 25 : 24;
    if (key.find("scanlinesd") != std::string::npos || key.find("scanline") != std::string::npos)
        return 17;
    if (key.find("dot-d4") != std::string::npos)
        return 21;
    if (key.find("dot-hv4") != std::string::npos)
        return 22;
    if (key.find("dot") != std::string::npos)
        return 21;
    if (key.find("bloom") != std::string::npos ||
        key.find("luna") != std::string::npos ||
        key.find("nataa") != std::string::npos)
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
