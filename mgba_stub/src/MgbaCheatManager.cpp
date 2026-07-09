#include "mgba_stub/MgbaCheatManager.hpp"

#include <mgba/core/cheats.h>
#include <mgba/core/core.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace beiklive::mgba_stub {
namespace {

std::string trim(std::string value)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string unquoteValue(std::string value)
{
    value = trim(std::move(value));
    if (value.size() < 2 || value.front() != '"' || value.back() != '"')
        return value;

    std::string out;
    out.reserve(value.size() - 2);
    for (std::size_t i = 1; i + 1 < value.size(); ++i)
    {
        if (value[i] == '\\' && i + 2 < value.size())
            out.push_back(value[++i]);
        else
            out.push_back(value[i]);
    }
    return out;
}

std::string quoteValue(const std::string& value)
{
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char c : value)
    {
        if (c == '"' || c == '\\')
            out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

bool parseBool(std::string value, bool fallback)
{
    value = trim(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (value == "1" || value == "true" || value == "on" || value == "yes")
        return true;
    if (value == "0" || value == "false" || value == "off" || value == "no")
        return false;
    return fallback;
}

std::map<std::string, std::string> parseConfig(std::istream& input)
{
    std::map<std::string, std::string> result;
    std::string line;
    while (std::getline(input, line))
    {
        line = trim(std::move(line));
        if (line.empty() || line.front() == '#')
            continue;
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = trim(line.substr(0, eq));
        if (!key.empty())
            result[key] = unquoteValue(line.substr(eq + 1));
    }
    return result;
}

std::vector<std::string> splitCodeLines(const std::string& code)
{
    std::vector<std::string> lines;
    std::string normalized = code;
    for (char& c : normalized)
    {
        if (c == '\r' || c == '\n' || c == ';')
            c = '+';
    }
    std::stringstream stream(normalized);
    std::string item;
    while (std::getline(stream, item, '+'))
    {
        item = trim(std::move(item));
        if (!item.empty())
            lines.push_back(item);
    }
    return lines;
}

bool isHexText(const std::string& value)
{
    if (value.empty())
        return false;
    for (unsigned char c : value)
    {
        if (!std::isxdigit(c))
            return false;
    }
    return true;
}

std::string detectCodeType(const std::string& code)
{
    bool hasRaw = false;
    bool hasGs = false;
    bool hasGbGameGenie = false;
    bool unknown = false;

    for (std::string line : splitCodeLines(code))
    {
        if (line.find('-') != std::string::npos)
        {
            hasGbGameGenie = true;
            continue;
        }
        if (line.find(':') != std::string::npos)
        {
            hasRaw = true;
            continue;
        }

        std::stringstream stream(line);
        std::string a;
        std::string b;
        std::string c;
        stream >> a >> b >> c;
        if (c.empty() && a.size() == 8 && b.size() == 4 && isHexText(a) && isHexText(b))
            hasRaw = true;
        else if (c.empty() && a.size() == 8 && b.size() == 8 && isHexText(a) && isHexText(b))
            hasGs = true;
        else if (b.empty() && a.size() == 8 && isHexText(a))
            hasGs = true;
        else
            unknown = true;
    }

    const int knownKinds = (hasRaw ? 1 : 0) + (hasGs ? 1 : 0) + (hasGbGameGenie ? 1 : 0);
    if (knownKinds > 1)
        return "Mixed";
    if (hasRaw)
        return "Raw";
    if (hasGs)
        return "GS";
    if (hasGbGameGenie)
        return "GG";
    return unknown ? "Auto" : "";
}

void clearCoreCheats(mCheatDevice* device)
{
    if (!device)
        return;

    while (mCheatSetsSize(&device->cheats) > 0)
    {
        mCheatSet* set = *mCheatSetsGetPointer(&device->cheats, 0);
        if (!set)
        {
            mCheatSetsShift(&device->cheats, 0, 1);
            continue;
        }
        set->enabled = false;
        mCheatRefresh(device, set);
        mCheatRemoveSet(device, set);
        mCheatSetDeinit(set);
    }
}

} // namespace

std::string DefaultCheatPath(const std::string& romPath)
{
    const std::string stem = std::filesystem::path(romPath).stem().string();
    return (std::filesystem::path("sdmc:/GBAStation/cheats") /
            ((stem.empty() ? std::string("game") : stem) + ".cht")).string();
}

bool LoadRetroArchCheats(const std::string& path, std::vector<MgbaCheatEntry>& out)
{
    out.clear();
    if (path.empty() || !std::filesystem::exists(path))
        return true;

    std::ifstream input(path);
    if (!input)
        return false;

    const auto kv = parseConfig(input);
    const auto totalIt = kv.find("cheats");
    if (totalIt == kv.end())
        return true;

    unsigned total = 0;
    try
    {
        total = static_cast<unsigned>(std::stoul(totalIt->second, nullptr, 0));
    }
    catch (...)
    {
        return false;
    }

    out.reserve(total);
    for (unsigned i = 0; i < total; ++i)
    {
        const std::string prefix = "cheat" + std::to_string(i) + "_";
        MgbaCheatEntry entry;
        const auto descIt = kv.find(prefix + "desc");
        const auto codeIt = kv.find(prefix + "code");
        const auto enableIt = kv.find(prefix + "enable");
        const auto handlerIt = kv.find(prefix + "handler");
        entry.name = descIt != kv.end() ? descIt->second : ("cheat" + std::to_string(i));
        entry.code = codeIt != kv.end() ? codeIt->second : "";
        entry.codeType = detectCodeType(entry.code);
        entry.enabled = enableIt != kv.end() ? parseBool(enableIt->second, true) : true;
        if (handlerIt != kv.end() && trim(handlerIt->second) == "1")
        {
            entry.valid = false;
            entry.diagnostic = "frontend memory patch";
        }
        if (entry.code.empty())
        {
            entry.valid = false;
            entry.diagnostic = "empty code";
        }
        out.push_back(std::move(entry));
    }
    return true;
}

bool SaveRetroArchCheats(const std::string& path, const std::vector<MgbaCheatEntry>& entries)
{
    if (path.empty())
        return false;

    std::ostringstream text;
    text << "cheats = " << entries.size() << "\n\n";
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        text << "cheat" << i << "_desc = " << quoteValue(entries[i].name) << "\n";
        text << "cheat" << i << "_enable = " << (entries[i].enabled ? "true" : "false") << "\n";
        text << "cheat" << i << "_code = " << quoteValue(entries[i].code) << "\n";
        text << "cheat" << i << "_handler = 0\n\n";
    }
    const std::string payload = text.str();

    std::vector<std::string> candidates;
    candidates.push_back(path);
    if (path.rfind("sdmc:/", 0) == 0)
        candidates.push_back(path.substr(5));
    else if (!path.empty() && path.front() == '/')
        candidates.push_back("sdmc:" + path);

    std::error_code ec;
    for (const std::string& candidate : candidates)
    {
        const auto parent = std::filesystem::path(candidate).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent, ec);

        FILE* file = std::fopen(candidate.c_str(), "wb");
        if (!file)
            continue;
        const std::size_t written = std::fwrite(payload.data(), 1, payload.size(), file);
        const bool closeOk = std::fclose(file) == 0;
        if (written == payload.size() && closeOk)
            return true;
    }
    return false;
}

std::vector<MgbaCheatItem> BuildCheatMenuItems(const std::vector<MgbaCheatEntry>& entries)
{
    std::vector<MgbaCheatItem> items;
    items.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        MgbaCheatItem item;
        item.type = MgbaCheatItem::Type::Code;
        item.name = entries[i].name;
        item.enabled = entries[i].enabled;
        item.code = entries[i].code;
        item.codeType = entries[i].codeType.empty() ? detectCodeType(entries[i].code) : entries[i].codeType;
        item.valid = entries[i].valid;
        item.diagnostic = entries[i].diagnostic;
        item.entryIndex = static_cast<int>(i);
        items.push_back(std::move(item));
    }
    return items;
}

void UpdateCheatsFromMenuItems(const std::vector<MgbaCheatItem>& items,
                               std::vector<MgbaCheatEntry>& entries)
{
    for (const auto& item : items)
    {
        if (item.entryIndex < 0 || item.entryIndex >= static_cast<int>(entries.size()))
            continue;
        auto& entry = entries[static_cast<std::size_t>(item.entryIndex)];
        entry.name = item.name;
        entry.code = item.code;
        entry.codeType = detectCodeType(item.code);
        entry.enabled = item.enabled;
    }
}

MgbaCheatApplyResult ApplyCheatsToCore(mCore* core, std::vector<MgbaCheatEntry>& entries)
{
    MgbaCheatApplyResult result;
    if (!core || !core->cheatDevice)
    {
        result.ok = false;
        return result;
    }

    mCheatDevice* device = core->cheatDevice(core);
    if (!device || !device->createSet)
    {
        result.ok = false;
        return result;
    }

    clearCoreCheats(device);

    for (auto& entry : entries)
    {
        if (!entry.valid && entry.diagnostic == "frontend memory patch")
        {
            entry.enabled = false;
            ++result.invalidCount;
            continue;
        }
        entry.valid = true;
        entry.diagnostic.clear();
        entry.codeType = detectCodeType(entry.code);
        const auto lines = splitCodeLines(entry.code);
        if (lines.empty())
        {
            entry.valid = false;
            entry.enabled = false;
            entry.diagnostic = "empty code";
            ++result.invalidCount;
            continue;
        }

        mCheatSet* set = device->createSet(device, entry.name.empty() ? "cheat" : entry.name.c_str());
        if (!set)
        {
            entry.valid = false;
            entry.enabled = false;
            entry.diagnostic = "create failed";
            result.ok = false;
            ++result.invalidCount;
            continue;
        }

        bool valid = true;
        for (const std::string& line : lines)
        {
            if (!mCheatAddLine(set, line.c_str(), 0))
            {
                valid = false;
                break;
            }
        }

        if (!valid)
        {
            entry.valid = false;
            entry.enabled = false;
            entry.diagnostic = "parse failed";
            mCheatSetDeinit(set);
            ++result.invalidCount;
            continue;
        }

        set->enabled = entry.enabled;
        mCheatAddSet(device, set);
        mCheatRefresh(device, set);
        if (entry.enabled)
            ++result.appliedCount;
    }

    return result;
}

} // namespace beiklive::mgba_stub
