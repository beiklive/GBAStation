#include "mgba_stub/MgbaCheatManager.hpp"

#include <mgba/core/cheats.h>
#include <mgba/core/core.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
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

void appendCheatLog(const std::string& message)
{
    const char* candidates[] = {
        "sdmc:/GBAStation/logs/mgba_cheats.log",
        "/GBAStation/logs/mgba_cheats.log",
    };

    std::time_t now = std::time(nullptr);
    std::tm tm {};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char timestamp[32] {};
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

    for (const char* path : candidates)
    {
        std::error_code ec;
        const auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent, ec);

        FILE* file = std::fopen(path, "ab");
        if (!file)
            continue;
        std::fprintf(file, "[%s] %s\n", timestamp, message.c_str());
        std::fclose(file);
        return;
    }
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

std::vector<std::string> splitCodeTokens(const std::string& code)
{
    std::vector<std::string> tokens;
    std::string normalized = code;
    for (char& c : normalized)
    {
        if (c == '\r' || c == '\n' || c == ';' || std::isspace(static_cast<unsigned char>(c)) != 0)
            c = '+';
    }
    std::stringstream stream(normalized);
    std::string item;
    while (std::getline(stream, item, '+'))
    {
        item = trim(std::move(item));
        if (!item.empty())
            tokens.push_back(item);
    }
    return tokens;
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

bool isHexTextWithSize(const std::string& value, std::size_t size)
{
    return value.size() == size && isHexText(value);
}

std::string upperAscii(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

std::string normalizeCodeType(std::string value)
{
    value = upperAscii(trim(std::move(value)));
    if (value == "RAW" || value == "VBA")
        return "RAW";
    if (value == "GS" || value == "CB" || value == "GS/CB" || value == "GAMESHARK" ||
        value == "CODEBREAKER")
        return "GS/CB";
    if (value == "GG" || value == "GAMEGENIE")
        return "GG";
    if (value == "MIXED")
        return "Mixed";
    return {};
}

bool codeTypeForcesRaw(const std::string& value)
{
    return normalizeCodeType(value) == "RAW";
}

bool codeTypeForcesGsCb(const std::string& value)
{
    return normalizeCodeType(value) == "GS/CB";
}

bool isLikelyGbaRawAddress(const std::string& value)
{
    if (!isHexTextWithSize(value, 8) || value[0] != '0')
        return false;
    const char high = static_cast<char>(std::toupper(static_cast<unsigned char>(value[1])));
    return high >= '2' && high <= 'E';
}

std::vector<std::string> splitGbaCodeLines(const std::string& code, const std::string& codeType = {})
{
    std::vector<std::string> lines;
    const auto tokens = splitCodeTokens(code);
    lines.reserve(tokens.size());
    const bool forceRaw = codeTypeForcesRaw(codeType);
    const bool forceGsCb = codeTypeForcesGsCb(codeType);
    for (std::size_t i = 0; i < tokens.size(); ++i)
    {
        const std::string& token = tokens[i];
        if (isHexTextWithSize(token, 8) && i + 1 < tokens.size())
        {
            const std::string& value = tokens[i + 1];
            if (isHexTextWithSize(value, 2) || isHexTextWithSize(value, 4) || isHexTextWithSize(value, 8))
            {
                const bool raw =
                    forceRaw ||
                    (!forceGsCb &&
                     (value.size() == 2 ||
                      (value.size() == 4 && isLikelyGbaRawAddress(token))));
                lines.push_back(raw ? (token + ":" + value) : (token + " " + value));
                ++i;
                continue;
            }
        }
        lines.push_back(token);
    }
    return lines;
}

std::vector<std::string> splitCodeLinesForPlatform(const std::string& code,
                                                   int platform,
                                                   const std::string& codeType = {})
{
    if (platform == mPLATFORM_GBA)
        return splitGbaCodeLines(code, codeType);
    return splitCodeTokens(code);
}

std::string detectCodeType(const std::string& code)
{
    bool hasRaw = false;
    bool hasGs = false;
    bool hasGbGameGenie = false;
    bool unknown = false;

    for (std::string line : splitGbaCodeLines(code))
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
        if (c.empty() && a.size() == 8 && (b.size() == 4 || b.size() == 8) && isHexText(a) && isHexText(b))
            hasGs = true;
        else if (c.empty() && a.size() == 8 && b.size() == 4 && isHexText(a) && isHexText(b))
            hasRaw = true;
        else if (b.empty() && a.size() == 8 && isHexText(a))
            hasGs = true;
        else
            unknown = true;
    }

    const int knownKinds = (hasRaw ? 1 : 0) + (hasGs ? 1 : 0) + (hasGbGameGenie ? 1 : 0);
    if (knownKinds > 1)
        return "Mixed";
    if (hasRaw)
        return "RAW";
    if (hasGs)
        return "GS/CB";
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

void AppendCheatLog(const std::string& message)
{
    appendCheatLog(message);
}

bool LoadRetroArchCheats(const std::string& path, std::vector<MgbaCheatEntry>& out)
{
    out.clear();
    if (path.empty() || !std::filesystem::exists(path))
    {
        appendCheatLog("load skipped: path missing or file does not exist: " + path);
        return true;
    }

    std::ifstream input(path);
    if (!input)
    {
        appendCheatLog("load failed: cannot open " + path);
        return false;
    }

    const auto kv = parseConfig(input);
    const auto totalIt = kv.find("cheats");
    if (totalIt == kv.end())
    {
        appendCheatLog("load ok: no cheats field in " + path);
        return true;
    }

    unsigned total = 0;
    try
    {
        total = static_cast<unsigned>(std::stoul(totalIt->second, nullptr, 0));
    }
    catch (...)
    {
        appendCheatLog("load failed: invalid cheats count in " + path + ", value=" + totalIt->second);
        return false;
    }

    out.reserve(total);
    for (unsigned i = 0; i < total; ++i)
    {
        const std::string prefix = "cheat" + std::to_string(i) + "_";
        MgbaCheatEntry entry;
        const auto descIt = kv.find(prefix + "desc");
        const auto codeIt = kv.find(prefix + "code");
        const auto handlerIt = kv.find(prefix + "handler");
        entry.name = descIt != kv.end() ? descIt->second : ("cheat" + std::to_string(i));
        entry.code = codeIt != kv.end() ? codeIt->second : "";
        entry.codeType = normalizeCodeType(entry.codeType);
        if (entry.codeType.empty())
            entry.codeType = detectCodeType(entry.code);
        entry.enabled = false;
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
    appendCheatLog("load ok: path=" + path + ", total=" + std::to_string(out.size()) +
                   ", all runtime switches forced off");
    return true;
}

bool SaveRetroArchCheats(const std::string& path, const std::vector<MgbaCheatEntry>& entries)
{
    if (path.empty())
    {
        appendCheatLog("save failed: empty path");
        return false;
    }

    std::ostringstream text;
    text << "cheats = " << entries.size() << "\n\n";
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        text << "cheat" << i << "_desc = " << quoteValue(entries[i].name) << "\n";
        text << "cheat" << i << "_enable = false\n";
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
        {
            appendCheatLog("save ok: path=" + candidate + ", total=" + std::to_string(entries.size()) +
                           ", enable fields forced false");
            return true;
        }
        appendCheatLog("save failed: short write path=" + candidate +
                       ", written=" + std::to_string(written) +
                       ", expected=" + std::to_string(payload.size()) +
                       ", closeOk=" + std::to_string(closeOk ? 1 : 0));
    }
    appendCheatLog("save failed: cannot write " + path);
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
        entry.codeType = normalizeCodeType(item.codeType);
        if (entry.codeType.empty())
            entry.codeType = detectCodeType(item.code);
        entry.enabled = item.enabled;
    }
}

MgbaCheatApplyResult ApplyCheatsToDevice(mCheatDevice* device,
                                         int platform,
                                         std::vector<MgbaCheatEntry>& entries,
                                         const char* sourceLabel)
{
    MgbaCheatApplyResult result;
    int enabledCount = 0;
    for (const auto& entry : entries)
    {
        if (entry.enabled)
            ++enabledCount;
    }

    auto finishWithFailure = [&](const std::string& reason) {
        result.ok = false;
        result.diagnostic = reason;
        appendCheatLog("apply failed: " + reason +
                       ", total=" + std::to_string(entries.size()) +
                       ", enabled=" + std::to_string(enabledCount) +
                       ", source=" + (sourceLabel ? sourceLabel : "unknown"));
        return result;
    };

    if (!device || !device->createSet)
        return finishWithFailure(!device ? "cheat device is null" : "cheat device createSet is null");

    appendCheatLog("apply begin: total=" + std::to_string(entries.size()) +
                   ", enabled=" + std::to_string(enabledCount) +
                   ", platform=" + std::to_string(platform) +
                   ", deviceReady=1, source=" + (sourceLabel ? sourceLabel : "unknown"));

    clearCoreCheats(device);

    for (std::size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
    {
        auto& entry = entries[entryIndex];
        if (!entry.valid && entry.diagnostic == "frontend memory patch")
        {
            entry.enabled = false;
            ++result.invalidCount;
            appendCheatLog("apply skip invalid frontend patch: index=" + std::to_string(entryIndex) +
                           ", name=" + entry.name);
            continue;
        }
        entry.valid = true;
        entry.diagnostic.clear();
        entry.codeType = normalizeCodeType(entry.codeType);
        if (entry.codeType.empty())
            entry.codeType = detectCodeType(entry.code);
        if (!entry.enabled)
            continue;

        const auto lines = splitCodeLinesForPlatform(entry.code, platform, entry.codeType);
        if (lines.empty())
        {
            entry.valid = false;
            entry.enabled = false;
            entry.diagnostic = "empty code";
            ++result.invalidCount;
            appendCheatLog("apply invalid: empty code index=" + std::to_string(entryIndex) +
                           ", name=" + entry.name);
            continue;
        }

        mCheatSet* set = device->createSet(device, entry.name.empty() ? "cheat" : entry.name.c_str());
        if (!set)
        {
            entry.valid = false;
            entry.enabled = false;
            entry.diagnostic = "create failed";
            result.ok = false;
            result.diagnostic = "createSet returned null";
            ++result.invalidCount;
            appendCheatLog("apply failed: createSet returned null, index=" + std::to_string(entryIndex) +
                           ", name=" + entry.name +
                           ", codeType=" + entry.codeType +
                           ", lines=" + std::to_string(lines.size()));
            continue;
        }

        bool valid = true;
        std::string failedLine;
        for (const std::string& line : lines)
        {
            if (!mCheatAddLine(set, line.c_str(), 0))
            {
                valid = false;
                failedLine = line;
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
            appendCheatLog("apply invalid: parse failed, index=" + std::to_string(entryIndex) +
                           ", name=" + entry.name +
                           ", codeType=" + entry.codeType +
                           ", failedLine=" + failedLine +
                           ", rawCode=" + entry.code);
            continue;
        }

        set->enabled = entry.enabled;
        mCheatAddSet(device, set);
        mCheatRefresh(device, set);
        if (entry.enabled)
        {
            ++result.appliedCount;
            appendCheatLog("apply ok entry: index=" + std::to_string(entryIndex) +
                           ", name=" + entry.name +
                           ", codeType=" + entry.codeType +
                           ", lines=" + std::to_string(lines.size()));
        }
    }

    appendCheatLog("apply done: ok=" + std::to_string(result.ok ? 1 : 0) +
                   ", applied=" + std::to_string(result.appliedCount) +
                   ", invalid=" + std::to_string(result.invalidCount) +
                   ", diagnostic=" + result.diagnostic);
    return result;
}

MgbaCheatApplyResult ApplyCheatsToCore(mCore* core, std::vector<MgbaCheatEntry>& entries)
{
    MgbaCheatApplyResult result;
    int enabledCount = 0;
    for (const auto& entry : entries)
    {
        if (entry.enabled)
            ++enabledCount;
    }
    if (!core || !core->cheatDevice)
    {
        result.ok = false;
        result.diagnostic = !core ? "core is null" : "core->cheatDevice is null";
        appendCheatLog("apply failed: " + result.diagnostic +
                       ", total=" + std::to_string(entries.size()) +
                       ", enabled=" + std::to_string(enabledCount) +
                       ", source=core");
        return result;
    }
    mCheatDevice* device = core->cheatDevice(core);
    int platform = -1;
    if (core->platform)
        platform = static_cast<int>(core->platform(core));
    return ApplyCheatsToDevice(device, platform, entries, "core");
}

} // namespace beiklive::mgba_stub
