#include "nds_stub/NdsCheatDatabase.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "../../third_party/ArcDelta_melonDS/src/CRC32.h"
#include "nds_stub/StubLog.hpp"

namespace beiklive::nds_stub {
namespace {

struct DatEntryInfo {
    std::uint32_t gameCode = 0;
    std::uint32_t checksum = 0;
    std::uint32_t offset = 0;
};

struct CategoryFrame {
    int index = -1;
    int remaining = 0;
};

std::uint32_t readLe32(const std::vector<std::uint8_t>& data, std::size_t offset)
{
    if (offset + 4 > data.size())
        return 0;
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

bool readFile(const std::string& path, std::vector<std::uint8_t>& out)
{
    out.clear();
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size <= 0)
        return false;
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(out.data()), size);
    return file.good();
}

std::string findUsrCheatDat()
{
    const char* candidates[] = {
        "sdmc:/GBAStation/cheats/usrcheat.dat",
        "/GBAStation/cheats/usrcheat.dat",
    };
    std::vector<std::uint8_t> probe;
    for (const char* path : candidates)
    {
        if (readFile(path, probe))
            return path;
    }
    return {};
}

std::string readNtString(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    const std::size_t start = pos;
    while (pos < data.size() && data[pos] != 0)
        ++pos;
    std::string text(reinterpret_cast<const char*>(data.data() + start), pos - start);
    if (pos < data.size())
        ++pos;
    return text;
}

void align4(std::size_t& pos)
{
    pos = (pos + 3u) & ~std::size_t(3u);
}

void consumeParentSlot(std::vector<CategoryFrame>& stack)
{
    if (stack.empty())
        return;

    if (stack.back().remaining > 0)
        --stack.back().remaining;

    while (!stack.empty() && stack.back().remaining <= 0)
        stack.pop_back();
}

bool parseItems(const std::vector<std::uint8_t>& data,
                std::size_t& pos,
                std::uint32_t count,
                std::vector<NdsCheatItem>& out)
{
    std::vector<CategoryFrame> categoryStack;

    for (std::uint32_t i = 0; i < count; ++i)
    {
        if (pos + 4 > data.size())
            return false;

        const std::uint32_t flags = readLe32(data, pos);
        pos += 4;
        const std::uint32_t totalLen = flags & 0x00FFFFFFu;
        const bool isCategory = (flags & (1u << 28)) != 0;
        const bool enabled = (flags & (1u << 24)) != 0;

        std::string rawName = readNtString(data, pos);
        const std::string desc = readNtString(data, pos);
        align4(pos);
        std::string name = rawName;
        if (name.empty())
            name = desc.empty() ? (isCategory ? "未命名目录" : "未命名金手指") : desc;

        const int parent = categoryStack.empty() ? -1 : categoryStack.back().index;
        const int depth = static_cast<int>(categoryStack.size());

        if (isCategory)
        {
            if (totalLen == 0 || totalLen >= 0x10000u)
                return false;

            NdsCheatItem item;
            item.type = NdsCheatItem::Type::Category;
            item.name = std::move(name);
            item.parent = parent;
            item.depth = depth;
            item.expanded = false;
            const int index = static_cast<int>(out.size());
            out.push_back(std::move(item));
            consumeParentSlot(categoryStack);
            categoryStack.push_back(CategoryFrame{index, static_cast<int>(totalLen)});
            continue;
        }

        if (pos + 4 > data.size())
            return false;
        const std::uint32_t codeLen = readLe32(data, pos);
        pos += 4;

        std::uint32_t expectedLen = static_cast<std::uint32_t>(rawName.length() + 1 + desc.length() + 1);
        expectedLen = ((expectedLen + 3u) >> 2) + 1u + codeLen;
        if (expectedLen != totalLen ||
            codeLen > 0x100000u ||
            (codeLen & 1u) != 0 ||
            pos + static_cast<std::size_t>(codeLen) * 4u > data.size())
            return false;

        NdsCheatItem item;
        item.type = NdsCheatItem::Type::Code;
        item.name = std::move(name);
        item.parent = parent;
        item.depth = depth;
        item.enabled = enabled;
        item.words.reserve(codeLen);
        for (std::uint32_t word = 0; word < codeLen; ++word)
        {
            item.words.push_back(readLe32(data, pos));
            pos += 4;
        }
        out.push_back(std::move(item));
        consumeParentSlot(categoryStack);
    }
    return true;
}

bool parseCheatsAtOffset(const std::vector<std::uint8_t>& data,
                         const DatEntryInfo& info,
                         NdsCheatLoadResult& result)
{
    if (info.offset < 0x100 || info.offset >= data.size())
        return false;

    std::size_t pos = info.offset;
    result.gameName = readNtString(data, pos);
    align4(pos);
    if (pos + 36 > data.size())
        return false;

    const std::uint32_t flags = readLe32(data, pos);
    pos += 36;
    const std::uint32_t itemCount = flags & 0x00FFFFFFu;
    result.items.clear();
    result.items.reserve(std::min<std::uint32_t>(itemCount, 1024u));
    return parseItems(data, pos, itemCount, result.items);
}

bool readRomIdentity(const std::string& romPath, std::uint32_t& gameCode, std::uint32_t& checksum)
{
    std::array<std::uint8_t, 512> header {};
    std::ifstream rom(romPath, std::ios::binary);
    if (!rom)
        return false;
    rom.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (rom.gcount() != static_cast<std::streamsize>(header.size()))
        return false;

    gameCode = static_cast<std::uint32_t>(header[12]) |
               (static_cast<std::uint32_t>(header[13]) << 8) |
               (static_cast<std::uint32_t>(header[14]) << 16) |
               (static_cast<std::uint32_t>(header[15]) << 24);
    checksum = ~CRC32(header.data(), static_cast<int>(header.size()));
    return true;
}

} // namespace

NdsCheatLoadResult LoadUsrCheatDatForRom(const std::string& romPath)
{
    NdsCheatLoadResult result;
    result.sourcePath = findUsrCheatDat();
    if (result.sourcePath.empty())
    {
        appendStubLog("GBAStationNDSStub: usrcheat.dat not found");
        return result;
    }
    result.databaseFound = true;

    std::uint32_t gameCode = 0;
    std::uint32_t checksum = 0;
    if (!readRomIdentity(romPath, gameCode, checksum))
    {
        appendStubLog("GBAStationNDSStub: usrcheat rom identity failed rom=%s", romPath.c_str());
        return result;
    }

    std::vector<std::uint8_t> data;
    if (!readFile(result.sourcePath, data) || data.size() < 0x110 ||
        std::memcmp(data.data(), "R4 CheatCode", 12) != 0)
    {
        appendStubLog("GBAStationNDSStub: usrcheat invalid path=%s", result.sourcePath.c_str());
        return result;
    }
    const std::uint32_t version = readLe32(data, 12);

    std::vector<DatEntryInfo> matches;
    for (std::size_t pos = 0x100; pos + 16 <= data.size(); pos += 16)
    {
        DatEntryInfo info;
        info.gameCode = readLe32(data, pos);
        info.checksum = readLe32(data, pos + 4);
        info.offset = readLe32(data, pos + 8);
        if (info.gameCode == 0)
            break;
        if (info.gameCode == gameCode)
            matches.push_back(info);
    }

    if (matches.empty())
    {
        appendStubLog("GBAStationNDSStub: usrcheat no game match gameCode=%08X checksum=%08X path=%s",
                      gameCode,
                      checksum,
                      result.sourcePath.c_str());
        return result;
    }

    std::vector<const DatEntryInfo*> candidates;
    for (const auto& match : matches)
    {
        if (match.checksum == checksum)
        {
            candidates.push_back(&match);
            break;
        }
    }
    for (const auto& match : matches)
    {
        if (match.checksum != checksum)
            candidates.push_back(&match);
    }

    const DatEntryInfo* selected = candidates.empty() ? &matches.front() : candidates.front();
    for (const DatEntryInfo* candidate : candidates)
    {
        NdsCheatLoadResult parsed;
        if (parseCheatsAtOffset(data, *candidate, parsed))
        {
            selected = candidate;
            result.gameName = std::move(parsed.gameName);
            result.items = std::move(parsed.items);
            result.gameMatched = true;
            break;
        }

        appendStubLog("GBAStationNDSStub: usrcheat parse failed offset=%08X checksum=%08X partialItems=%d game=%s",
                      candidate->offset,
                      candidate->checksum,
                      static_cast<int>(parsed.items.size()),
                      parsed.gameName.c_str());
    }
    if (!result.gameMatched)
        result.items.clear();

    appendStubLog("GBAStationNDSStub: usrcheat load path=%s version=%08X gameCode=%08X checksum=%08X entryChecksum=%08X entryOffset=%08X matched=%d items=%d game=%s",
                  result.sourcePath.c_str(),
                  version,
                  gameCode,
                  checksum,
                  selected->checksum,
                  selected->offset,
                  result.gameMatched ? 1 : 0,
                  static_cast<int>(result.items.size()),
                  result.gameName.c_str());
    return result;
}

} // namespace beiklive::nds_stub
