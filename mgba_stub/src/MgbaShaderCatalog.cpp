#include "mgba_stub/MgbaShaderCatalog.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace beiklive::mgba_stub {
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
    return value.size() >= p.size() && std::equal(p.begin(), p.end(), value.begin());
}

bool isRetroArchShader(const std::string& type)
{
    return startsWith(type, "RetroArch_");
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

void sortShaderEntries(std::vector<MgbaShaderListEntry>& entries)
{
    std::sort(entries.begin(), entries.end(), [](const MgbaShaderListEntry& lhs, const MgbaShaderListEntry& rhs) {
        const std::string a = lowerAscii(lhs.label);
        const std::string b = lowerAscii(rhs.label);
        if (a != b)
            return a < b;
        return lhs.label < rhs.label;
    });
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
        if (!isRetroArchShader(type) ||
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

const std::vector<std::string>& availableMgbaShaderTypes()
{
    static const std::vector<std::string> types = loadShaderTypes();
    return types;
}

bool isKnownMgbaShaderType(const std::string& type)
{
    const auto& types = availableMgbaShaderTypes();
    return std::find(types.begin(), types.end(), type) != types.end();
}

std::string normalizeMgbaShaderType(const std::string& type)
{
    if (isKnownMgbaShaderType(type))
        return type;

    const std::string oldRetro = oldRetroArchName(type);
    if (!oldRetro.empty() && isKnownMgbaShaderType(oldRetro))
        return oldRetro;

    const std::string key = MgbaShaderMatchKey(type);
    if (!key.empty())
    {
        const auto& types = availableMgbaShaderTypes();
        for (const auto& candidate : types)
        {
            if (MgbaShaderMatchKey(candidate) == key)
                return candidate;
        }
    }

    return kDefaultShaderType;
}

std::string MgbaShaderDisplayName(const std::string& type)
{
    return type.empty() ? kDefaultShaderType : type;
}

std::string MgbaShaderMatchKey(const std::string& type)
{
    if (isRetroArchShader(type))
        return "retroarch-" + canonicalStem(type.substr(10));
    return canonicalStem(type);
}

std::vector<MgbaShaderListEntry> MgbaShaderListEntries(const std::vector<std::string>& path)
{
    std::vector<MgbaShaderListEntry> result;
    const auto& types = availableMgbaShaderTypes();

    if (path.empty())
    {
        result.push_back({MgbaShaderListEntry::Kind::Directory, "RetroArch", {}, {"RetroArch"}});
        return result;
    }

    if (path.size() == 1 && path[0] == "RetroArch")
    {
        for (const auto& type : types)
            result.push_back({MgbaShaderListEntry::Kind::Shader, MgbaShaderDisplayName(type), type, path});
        sortShaderEntries(result);
    }
    return result;
}

std::vector<std::string> MgbaShaderListPathForType(const std::string& type)
{
    const std::string normalized = normalizeMgbaShaderType(type);
    if (isRetroArchShader(normalized))
        return {"RetroArch"};
    return {};
}

} // namespace beiklive::mgba_stub
