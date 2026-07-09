#include "emulator/mgba_native/MgbaCheatSystem.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace beiklive::mgba_native::cheats
{
namespace
{
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

std::string stripComment(const std::string& line)
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

std::unordered_map<std::string, std::string> parseConfig(const std::string& content)
{
    std::unordered_map<std::string, std::string> kv;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        line = stripComment(line);
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

bool hexOnly(const std::string& value)
{
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

bool hexOnlyWithSize(const std::string& value, size_t size)
{
    return value.size() == size && hexOnly(value);
}

std::string upperAscii(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

std::string normalizeCodeTypeValue(std::string value)
{
    value = upperAscii(trim(std::move(value)));
    if (value == "RAW" || value == "VBA")
        return "RAW";
    if (value == "GS" || value == "CB" || value == "GS/CB" ||
        value == "GAMESHARK" || value == "CODEBREAKER")
        return "GS/CB";
    if (value == "GG" || value == "GAMEGENIE")
        return "GG";
    if (value == "MIXED")
        return "Mixed";
    if (value == "AUTO")
        return "Auto";
    return {};
}

bool codeTypeForcesRaw(const std::string& value)
{
    return normalizeCodeTypeValue(value) == "RAW";
}

bool codeTypeForcesGsCb(const std::string& value)
{
    return normalizeCodeTypeValue(value) == "GS/CB";
}

bool isLikelyGbaRawAddress(const std::string& value)
{
    if (!hexOnlyWithSize(value, 8) || value[0] != '0')
        return false;
    const char high = static_cast<char>(std::toupper(static_cast<unsigned char>(value[1])));
    return high >= '2' && high <= 'E';
}

std::vector<std::string> splitCodeTokens(const std::string& code)
{
    std::vector<std::string> tokens;
    std::string current;
    for (unsigned char ch : code)
    {
        if (std::isspace(ch) || ch == '+' || ch == ',' || ch == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(trim(current));
                current.clear();
            }
            continue;
        }
        current.push_back(static_cast<char>(ch));
    }
    if (!current.empty())
        tokens.push_back(trim(current));
    return tokens;
}

std::vector<std::string> splitGbaCodeLines(const std::string& code, const std::string& codeType = {})
{
    std::vector<std::string> lines;
    const auto tokens = splitCodeTokens(code);
    lines.reserve(tokens.size());
    const bool forceRaw = codeTypeForcesRaw(codeType);
    const bool forceGsCb = codeTypeForcesGsCb(codeType);

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const std::string& token = tokens[i];
        if (hexOnlyWithSize(token, 8) && i + 1 < tokens.size())
        {
            const std::string& value = tokens[i + 1];
            if (hexOnlyWithSize(value, 2) || hexOnlyWithSize(value, 4) || hexOnlyWithSize(value, 8))
            {
                const bool raw =
                    forceRaw ||
                    (!forceGsCb &&
                     (value.size() == 2 || (value.size() == 4 && isLikelyGbaRawAddress(token))));
                lines.push_back(raw ? (token + ":" + value) : (token + " " + value));
                ++i;
                continue;
            }
        }

        if (token.find(':') == std::string::npos && token.find('-') == std::string::npos &&
            (token.size() == 10 || token.size() == 12) && hexOnly(token))
        {
            lines.push_back(token.substr(0, 8) + ":" + token.substr(8));
            continue;
        }

        lines.push_back(token);
    }
    return lines;
}

std::string detectCodeTypeValue(const std::string& code)
{
    bool hasRaw = false;
    bool hasGs = false;
    bool hasGbGameGenie = false;
    bool unknown = false;

    for (const std::string& line : splitGbaCodeLines(code))
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

        std::istringstream stream(line);
        std::string a;
        std::string b;
        std::string c;
        stream >> a >> b >> c;
        if (c.empty() && a.size() == 8 && (b.size() == 4 || b.size() == 8) && hexOnly(a) && hexOnly(b))
            hasGs = true;
        else if (b.empty() && a.size() == 8 && hexOnly(a))
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

bool looksLikeMgbaLine(const std::string& line)
{
    if (line.find(':') != std::string::npos ||
        line.find('-') != std::string::npos ||
        line.find('+') != std::string::npos)
        return true;

    std::istringstream iss(line);
    std::string first;
    std::string second;
    iss >> first >> second;
    if (first.size() == 8 && (second.size() == 2 || second.size() == 4 || second.size() == 8))
        return hexOnly(first) && hexOnly(second);
    return false;
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

std::vector<beiklive::CheatEntry> parsePlainText(const std::string& content)
{
    std::vector<beiklive::CheatEntry> out;
    std::istringstream iss(content);
    std::string line;
    size_t index = 0;

    while (std::getline(iss, line))
    {
        line = trim(stripComment(line));
        if (line.empty() || line[0] == '[' || line[0] == ';')
            continue;

        if (line[0] == '+' || line[0] == '-')
            line = trim(line.substr(1));
        if (!looksLikeMgbaLine(line))
            continue;

        beiklive::CheatEntry entry;
        entry.id = "mgba-plain:" + std::to_string(index++);
        entry.desc = line;
        entry.code = line;
        entry.codeType = detectCodeTypeValue(entry.code);
        entry.enabled = false;
        entry.sourceFormat = beiklive::CheatSourceFormat::PlainText;
        entry.payloadType = beiklive::CheatPayloadType::LibretroRaw;
        out.push_back(std::move(entry));
    }

    return out;
}

std::vector<beiklive::CheatEntry> parseRetroArchCht(const std::string& content)
{
    std::vector<beiklive::CheatEntry> result;
    const auto kv = parseConfig(content);
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
        const auto codeTypeIt = kv.find(prefix + "code_type");
        const auto typeIt = kv.find(prefix + "type");

        beiklive::CheatEntry entry;
        entry.id = "mgba-cht:" + std::to_string(i);
        entry.desc = descIt != kv.end() ? descIt->second : ("cheat" + std::to_string(i));
        entry.code = codeIt != kv.end() ? codeIt->second : "";
        entry.codeType = codeTypeIt != kv.end()
            ? normalizeCodeTypeValue(codeTypeIt->second)
            : (typeIt != kv.end() ? normalizeCodeTypeValue(typeIt->second) : "");
        if (entry.codeType.empty())
            entry.codeType = detectCodeTypeValue(entry.code);
        entry.enabled = false;
        entry.sourceFormat = beiklive::CheatSourceFormat::RetroArchCht;
        entry.payloadType = entry.code.empty()
            ? beiklive::CheatPayloadType::Unsupported
            : beiklive::CheatPayloadType::LibretroRaw;
        result.push_back(std::move(entry));
    }
    return result;
}
}

bool IsMgbaPlatform(int platform)
{
    using beiklive::enums::EmuPlatform;
    return platform == static_cast<int>(EmuPlatform::EmuGBA) ||
           platform == static_cast<int>(EmuPlatform::EmuGBC) ||
           platform == static_cast<int>(EmuPlatform::EmuGB);
}

std::string NormalizeCodeType(std::string value)
{
    return normalizeCodeTypeValue(std::move(value));
}

std::string DetectCodeType(const std::string& code)
{
    return detectCodeTypeValue(code);
}

LoadResult LoadCheats(const std::string& path)
{
    LoadResult result;
    if (path.empty() || !std::filesystem::exists(path))
        return result;

    const std::string content = readTextFile(path);
    if (content.empty())
        return result;

    result.loaded = true;
    result.editable = true;
    result.entries = content.find("cheats") == std::string::npos
        ? parsePlainText(content)
        : parseRetroArchCht(content);
    for (auto& entry : result.entries)
        entry.editable = entry.payloadType != beiklive::CheatPayloadType::Category;
    return result;
}

bool SaveChtFile(const std::string& path, const std::vector<beiklive::CheatEntry>& entries)
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
        f << "cheat" << outIndex << "_enable = false\n";
        f << "cheat" << outIndex << "_code = \"" << entry.code << "\"\n";
        const std::string codeType = normalizeCodeTypeValue(entry.codeType);
        if (!codeType.empty())
            f << "cheat" << outIndex << "_code_type = \"" << codeType << "\"\n";
        f << "cheat" << outIndex << "_handler = 0\n\n";
        ++outIndex;
    }
    return !!f;
}
}
