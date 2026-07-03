#include "core/cheat/CheatSystem.hpp"

#include "core/common.h"

#ifdef GBASTATION_ENABLE_MELONDS_CHEAT_DAT
#include "ARDatabaseDAT.h"
#include "CRC32.h"
#include "NDS_Header.h"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <variant>

namespace beiklive::cheat {
namespace {

std::string trim(std::string s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string trimConfigValue(std::string s)
{
    s = trim(std::move(s));
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
    return s;
}

std::string makeId(const std::string& prefix, size_t index)
{
    return prefix + ":" + std::to_string(index);
}

bool parseBool(std::string value, bool defaultValue)
{
    value = trimConfigValue(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "true" || value == "1" || value == "yes" || value == "on")
        return true;
    if (value == "false" || value == "0" || value == "no" || value == "off")
        return false;
    return defaultValue;
}

std::string stripTrailingComment(const std::string& line)
{
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '"')
            inQuote = !inQuote;
        else if (line[i] == '#' && !inQuote)
            return line.substr(0, i);
    }
    return line;
}

std::unordered_map<std::string, std::string> parseConfigText(const std::string& content)
{
    std::unordered_map<std::string, std::string> kv;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        line = stripTrailingComment(line);
        const auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        auto key = trim(line.substr(0, eq));
        auto value = trimConfigValue(line.substr(eq + 1));
        if (!key.empty())
            kv[std::move(key)] = std::move(value);
    }
    return kv;
}

bool hasHexOnly(const std::string& text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

bool looksLikePlainCheatLine(const std::string& line)
{
    if (line.find(':') != std::string::npos || line.find('+') != std::string::npos)
        return true;

    std::istringstream iss(line);
    std::string first;
    std::string second;
    iss >> first >> second;
    if (first.size() == 8 && (second.size() == 4 || second.size() == 8))
        return hasHexOnly(first) && hasHexOnly(second);
    return false;
}

std::vector<beiklive::CheatEntry> parsePlainText(const std::string& content)
{
    std::vector<beiklive::CheatEntry> out;
    std::istringstream iss(content);
    std::string line;
    size_t index = 0;

    while (std::getline(iss, line))
    {
        line = trim(stripTrailingComment(line));
        if (line.empty() || line[0] == '[' || line[0] == ';')
            continue;

        bool enabled = true;
        if (line[0] == '+' || line[0] == '-')
        {
            enabled = line[0] == '+';
            line = trim(line.substr(1));
        }
        if (!looksLikePlainCheatLine(line))
            continue;

        beiklive::CheatEntry entry;
        entry.id = makeId("plain", index++);
        entry.desc = line;
        entry.code = line;
        entry.enabled = enabled;
        entry.sourceFormat = beiklive::CheatSourceFormat::PlainText;
        entry.payloadType = beiklive::CheatPayloadType::LibretroRaw;
        out.push_back(std::move(entry));
    }

    return out;
}

std::string readTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
        return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string ndsWordsToText(const std::vector<uint32_t>& words)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    for (size_t i = 0; i + 1 < words.size(); i += 2)
    {
        if (i > 0)
            oss << '\n';
        oss << std::setw(8) << words[i] << ' ' << std::setw(8) << words[i + 1];
    }
    return oss.str();
}

#ifdef GBASTATION_ENABLE_MELONDS_CHEAT_DAT
std::vector<uint32_t> wordsFromMelonCode(const melonDS::ARCode& code)
{
    std::vector<uint32_t> words;
    words.reserve(code.Code.size());
    for (const auto word : code.Code)
        words.push_back(static_cast<uint32_t>(word));
    return words;
}

void appendNdsDatCheatsFromCat(const melonDS::ARCodeCat& cat,
                               std::vector<beiklive::CheatEntry>& out,
                               int depth,
                               int& groupCounter,
                               const std::string& parentId)
{
    int currentExclusiveGroup = -1;
    if (cat.OnlyOneCodeEnabled)
        currentExclusiveGroup = groupCounter++;

    size_t localIndex = 0;
    for (const auto& item : cat.Children)
    {
        if (std::holds_alternative<melonDS::ARCodeCat>(item))
        {
            const auto& childCat = std::get<melonDS::ARCodeCat>(item);
            const std::string catId = parentId + "/cat" + std::to_string(localIndex++);
            if (!childCat.Name.empty())
            {
                beiklive::CheatEntry category;
                category.id = catId;
                category.parentId = parentId;
                category.desc = std::string(static_cast<size_t>(std::max(0, depth)) * 2, ' ') + childCat.Name;
                category.enabled = false;
                category.editable = false;
                category.sourceFormat = beiklive::CheatSourceFormat::NdsUsrCheatDat;
                category.payloadType = beiklive::CheatPayloadType::Category;
                category.exclusiveGroup = currentExclusiveGroup;
                out.push_back(std::move(category));
            }
            appendNdsDatCheatsFromCat(childCat, out, depth + 1, groupCounter, catId);
            continue;
        }

        const auto& code = std::get<melonDS::ARCode>(item);
        if (code.Code.empty())
            continue;

        beiklive::CheatEntry entry;
        entry.id = parentId + "/code" + std::to_string(localIndex++);
        entry.parentId = parentId;
        entry.desc = code.Name.empty() ? code.Description : code.Name;
        if (entry.desc.empty())
            entry.desc = "NDS AR Code";
        entry.ndsWords = wordsFromMelonCode(code);
        entry.code = ndsWordsToText(entry.ndsWords);
        entry.enabled = code.Enabled;
        entry.editable = false;
        entry.sourceFormat = beiklive::CheatSourceFormat::NdsUsrCheatDat;
        entry.payloadType = beiklive::CheatPayloadType::MelonDsAr;
        entry.exclusiveGroup = currentExclusiveGroup;
        out.push_back(std::move(entry));
    }
}
#endif

} // namespace

std::string lowerExtension(const std::string& path)
{
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

bool isNdsUsrCheatDat(const std::string& path, int platform)
{
    return platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)
        && lowerExtension(path) == ".dat";
}

std::vector<std::string> extractHexTokens(const std::string& code)
{
    std::vector<std::string> tokens;
    std::string token;
    auto flush = [&]() {
        if (token.empty())
            return;
        if (token.size() > 8 && token.size() % 8 == 0)
        {
            for (size_t i = 0; i < token.size(); i += 8)
                tokens.push_back(token.substr(i, 8));
        }
        else
        {
            tokens.push_back(token);
        }
        token.clear();
    };

    for (char ch : code)
    {
        if (std::isxdigit(static_cast<unsigned char>(ch)))
            token.push_back(ch);
        else
            flush();
    }
    flush();
    return tokens;
}

bool parseU32Hex(const std::string& text, uint32_t& out)
{
    if (text.empty() || text.size() > 8)
        return false;
    try
    {
        size_t consumed = 0;
        unsigned long value = std::stoul(text, &consumed, 16);
        if (consumed != text.size())
            return false;
        out = static_cast<uint32_t>(value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string stateKey(const beiklive::CheatEntry& cheat)
{
    if (!cheat.id.empty())
        return cheat.id;
    return cheat.desc + "\n" + cheat.code;
}

std::vector<beiklive::CheatEntry> loadChtFile(const std::string& path)
{
    std::vector<beiklive::CheatEntry> result;
    if (path.empty() || !std::filesystem::exists(path))
        return result;

    const std::string content = readTextFile(path);
    if (content.empty())
        return result;

    if (content.find("cheats") == std::string::npos)
        return parsePlainText(content);

    const auto kv = parseConfigText(content);
    auto totalIt = kv.find("cheats");
    if (totalIt == kv.end())
        return parsePlainText(content);

    unsigned total = 0;
    try
    {
        total = static_cast<unsigned>(std::stoul(totalIt->second, nullptr, 0));
    }
    catch (...)
    {
        return result;
    }

    result.reserve(total);
    for (unsigned i = 0; i < total; ++i)
    {
        const auto prefix = "cheat" + std::to_string(i) + "_";
        const auto codeIt = kv.find(prefix + "code");
        const auto descIt = kv.find(prefix + "desc");
        const auto enableIt = kv.find(prefix + "enable");
        const auto handlerIt = kv.find(prefix + "handler");

        beiklive::CheatEntry entry;
        entry.id = makeId("cht", i);
        entry.desc = descIt != kv.end() ? descIt->second : ("cheat" + std::to_string(i));
        entry.code = codeIt != kv.end() ? codeIt->second : "";
        entry.enabled = enableIt != kv.end() ? parseBool(enableIt->second, true) : true;
        entry.sourceFormat = beiklive::CheatSourceFormat::RetroArchCht;
        entry.payloadType = entry.code.empty()
            ? beiklive::CheatPayloadType::Unsupported
            : beiklive::CheatPayloadType::LibretroRaw;

        if (handlerIt != kv.end())
        {
            try
            {
                const int handler = std::stoi(handlerIt->second, nullptr, 0);
                if (handler == 1)
                    entry.payloadType = beiklive::CheatPayloadType::FrontendMemoryPatch;
            }
            catch (...)
            {
            }
        }

        if (entry.payloadType == beiklive::CheatPayloadType::FrontendMemoryPatch)
        {
            entry.valid = false;
            entry.diagnostic = "Frontend memory patch cheats are parsed but not executable yet";
        }

        result.push_back(std::move(entry));
    }
    return result;
}

std::vector<beiklive::CheatEntry> loadNdsUsrCheatDat(const std::string& datPath,
                                                     const std::string& romPath)
{
    std::vector<beiklive::CheatEntry> result;
#ifndef GBASTATION_ENABLE_MELONDS_CHEAT_DAT
    (void)datPath;
    (void)romPath;
    return result;
#else
    if (datPath.empty() || romPath.empty())
        return result;
    if (!std::filesystem::exists(datPath) || !std::filesystem::exists(romPath))
        return result;

    std::array<melonDS::u8, 0x200> headerBytes {};
    melonDS::NDSHeader header {};
    {
        std::ifstream rom(romPath, std::ios::binary);
        if (!rom)
            return result;
        rom.read(reinterpret_cast<char*>(headerBytes.data()),
                 static_cast<std::streamsize>(headerBytes.size()));
        if (rom.gcount() != static_cast<std::streamsize>(headerBytes.size()))
            return result;
        std::memcpy(&header, headerBytes.data(), std::min(sizeof(header), headerBytes.size()));
    }

    melonDS::ARDatabaseDAT db(datPath);
    if (db.Error)
        return result;

    const melonDS::u32 gameCode = header.GameCodeAsU32();
    const melonDS::u32 checksum = ~melonDS::CRC32(headerBytes.data(),
                                                 static_cast<int>(headerBytes.size()),
                                                 0);
    auto entries = db.GetEntriesByGameCode(gameCode);
    if (entries.empty())
        return result;

    bool hasChecksumMatch = false;
    for (const auto& entry : entries)
    {
        if (entry.Checksum == checksum)
        {
            hasChecksumMatch = true;
            break;
        }
    }

    int groupCounter = 0;
    for (const auto& entry : entries)
    {
        if (hasChecksumMatch && entry.Checksum != checksum)
            continue;
        appendNdsDatCheatsFromCat(entry.RootCat, result, 0, groupCounter, "ndsdb");
        if (!hasChecksumMatch)
            break;
    }

    return result;
#endif
}

LoadResult loadCheats(const LoadRequest& request)
{
    LoadResult result;
    if (request.path.empty())
        return result;

    if (isNdsUsrCheatDat(request.path, request.platform))
    {
        result.format = beiklive::CheatSourceFormat::NdsUsrCheatDat;
        result.editable = false;
        result.entries = loadNdsUsrCheatDat(request.path, request.romPath);
    }
    else
    {
        result.format = lowerExtension(request.path) == ".cht"
            ? beiklive::CheatSourceFormat::RetroArchCht
            : beiklive::CheatSourceFormat::PlainText;
        result.editable = true;
        result.entries = loadChtFile(request.path);
    }

    for (auto& entry : result.entries)
    {
        entry.sourceFormat = entry.sourceFormat == beiklive::CheatSourceFormat::Unknown
            ? result.format
            : entry.sourceFormat;
        entry.editable = result.editable && entry.payloadType != beiklive::CheatPayloadType::Category;
    }
    return result;
}

bool saveChtFile(const std::string& path,
                 const std::vector<beiklive::CheatEntry>& entries)
{
    std::ofstream f(path);
    if (!f)
        return false;

    size_t count = 0;
    for (const auto& entry : entries)
    {
        if (entry.payloadType != beiklive::CheatPayloadType::Category)
            ++count;
    }

    f << "cheats = " << count << "\n\n";
    size_t outIndex = 0;
    for (const auto& entry : entries)
    {
        if (entry.payloadType == beiklive::CheatPayloadType::Category)
            continue;
        f << "cheat" << outIndex << "_desc = \"" << entry.desc << "\"\n";
        f << "cheat" << outIndex << "_enable = " << (entry.enabled ? "true" : "false") << "\n";
        f << "cheat" << outIndex << "_code = \"" << entry.code << "\"\n";
        f << "cheat" << outIndex << "_handler = 0\n\n";
        ++outIndex;
    }
    return !!f;
}

std::vector<beiklive::CheatEntry> filterRunnableCheats(
    const std::vector<beiklive::CheatEntry>& entries)
{
    std::vector<beiklive::CheatEntry> out;
    out.reserve(entries.size());
    for (const auto& entry : entries)
    {
        if (entry.payloadType == beiklive::CheatPayloadType::Category)
            continue;
        if (!entry.valid)
            continue;
        out.push_back(entry);
    }
    return out;
}

} // namespace beiklive::cheat
