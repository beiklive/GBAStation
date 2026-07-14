#include "nds_stub/NdsCheatDatabase.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "../../third_party/ArcDelta_melonDS/src/CRC32.h"
#include "nds_stub/StubLog.hpp"

namespace beiklive::nds_stub {
namespace {

constexpr std::uint64_t kDatHeaderSize = 0x100;
constexpr std::uint32_t kMaxCodeWords = 0x100000u;
constexpr std::size_t kMaxStringBytes = 1024u * 1024u;

struct DatEntryInfo {
    std::uint32_t gameCode = 0;
    std::uint32_t checksum = 0;
    std::uint32_t offset = 0;
};

struct CategoryFrame {
    int index = -1;
    int remaining = 0;
};

long long elapsedMs(const std::chrono::steady_clock::time_point& begin)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - begin).count();
}

std::uint32_t readLe32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

bool readLe32(std::istream& input, std::uint32_t& value)
{
    std::array<std::uint8_t, 4> bytes {};
    input.read(reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!input)
        return false;
    value = readLe32(bytes.data());
    return true;
}

bool seekTo(std::istream& input, std::uint64_t offset)
{
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        return false;
    input.clear();
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    return static_cast<bool>(input);
}

bool streamPosition(std::istream& input, std::uint64_t& position)
{
    const std::streampos pos = input.tellg();
    if (pos == std::streampos(-1))
        return false;
    position = static_cast<std::uint64_t>(pos);
    return true;
}

bool skipBytes(std::istream& input, std::uint64_t bytes, std::uint64_t fileSize)
{
    std::uint64_t pos = 0;
    if (!streamPosition(input, pos) ||
        bytes > fileSize - std::min(pos, fileSize) ||
        bytes > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    input.ignore(static_cast<std::streamsize>(bytes));
    return static_cast<bool>(input);
}

bool getFileSize(std::ifstream& file, std::uint64_t& size)
{
    file.clear();
    file.seekg(0, std::ios::end);
    const std::streampos end = file.tellg();
    if (end == std::streampos(-1))
        return false;
    size = static_cast<std::uint64_t>(end);
    return seekTo(file, 0);
}

std::string findUsrCheatDat()
{
    const char* candidates[] = {
        "sdmc:/GBAStation/cheats/usrcheat.dat",
        "/GBAStation/cheats/usrcheat.dat",
        "sdmc:/GBAStation/usrcheat.dat",
        "/GBAStation/usrcheat.dat",
    };
    for (const char* path : candidates)
    {
        std::ifstream probe(path, std::ios::binary);
        if (probe)
            return path;
    }
    return {};
}

bool readNtString(std::istream& input, std::string& text)
{
    text.clear();
    for (std::size_t i = 0; i < kMaxStringBytes; ++i)
    {
        char ch = 0;
        if (!input.get(ch))
            return false;
        if (ch == '\0')
            return true;
        text.push_back(ch);
    }
    return false;
}

bool align4(std::istream& input, std::uint64_t fileSize)
{
    std::uint64_t pos = 0;
    if (!streamPosition(input, pos))
        return false;
    const std::uint64_t aligned = (pos + 3u) & ~std::uint64_t(3u);
    if (aligned > fileSize)
        return false;
    return aligned == pos || seekTo(input, aligned);
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

bool parseItems(std::istream& input,
                std::uint64_t fileSize,
                std::uint32_t count,
                std::vector<NdsCheatItem>& out,
                int& skippedInvalidCodes)
{
    std::vector<CategoryFrame> categoryStack;

    for (std::uint32_t i = 0; i < count; ++i)
    {
        std::uint32_t flags = 0;
        if (!readLe32(input, flags))
            return false;

        const std::uint32_t totalLen = flags & 0x00FFFFFFu;
        const bool isCategory = (flags & (1u << 28)) != 0;
        const bool enabled = (flags & (1u << 24)) != 0;

        std::string rawName;
        std::string desc;
        if (!readNtString(input, rawName) ||
            !readNtString(input, desc) ||
            !align4(input, fileSize))
            return false;

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

        std::uint32_t codeLen = 0;
        if (!readLe32(input, codeLen))
            return false;

        std::uint32_t expectedLen = static_cast<std::uint32_t>(rawName.length() + 1 + desc.length() + 1);
        expectedLen = ((expectedLen + 3u) >> 2) + 1u + codeLen;
        const std::uint64_t codeBytes = static_cast<std::uint64_t>(codeLen) * 4u;
        if (expectedLen != totalLen ||
            codeLen > kMaxCodeWords)
            return false;

        if ((codeLen & 1u) != 0)
        {
            if (!skipBytes(input, codeBytes, fileSize))
                return false;
            ++skippedInvalidCodes;
            appendStubLog("GBAStationNDSStub: usrcheat skipped odd codeLen=%u item=%u name=%s",
                          codeLen,
                          i,
                          name.c_str());
            consumeParentSlot(categoryStack);
            continue;
        }

        NdsCheatItem item;
        item.type = NdsCheatItem::Type::Code;
        item.name = std::move(name);
        item.parent = parent;
        item.depth = depth;
        item.enabled = enabled;
        item.words.resize(codeLen);
        if (codeBytes > 0)
        {
            input.read(reinterpret_cast<char*>(item.words.data()),
                       static_cast<std::streamsize>(codeBytes));
            if (!input)
                return false;
        }
        out.push_back(std::move(item));
        consumeParentSlot(categoryStack);
    }
    return true;
}

bool parseCheatsAtOffset(std::ifstream& file,
                         std::uint64_t fileSize,
                         const DatEntryInfo& info,
                         NdsCheatLoadResult& result)
{
    if (info.offset < kDatHeaderSize || info.offset >= fileSize || !seekTo(file, info.offset))
        return false;

    if (!readNtString(file, result.gameName) || !align4(file, fileSize))
        return false;

    std::uint32_t flags = 0;
    if (!readLe32(file, flags) || !skipBytes(file, 32, fileSize))
        return false;

    const std::uint32_t itemCount = flags & 0x00FFFFFFu;
    result.items.clear();
    result.items.reserve(std::min<std::uint32_t>(itemCount, 1024u));
    result.skippedInvalidCodes = 0;
    return parseItems(file, fileSize, itemCount, result.items, result.skippedInvalidCodes);
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

    gameCode = readLe32(header.data() + 12);
    checksum = ~CRC32(header.data(), static_cast<int>(header.size()));
    return true;
}

} // namespace

NdsCheatLoadResult LoadUsrCheatDatForRom(const std::string& romPath)
{
    const auto loadBegin = std::chrono::steady_clock::now();
    NdsCheatLoadResult result;
    result.sourcePath = findUsrCheatDat();
    if (result.sourcePath.empty())
    {
        appendStubLog("GBAStationNDSStub: usrcheat.dat not found ms=%lld", elapsedMs(loadBegin));
        return result;
    }
    result.databaseFound = true;

    std::uint32_t gameCode = 0;
    std::uint32_t checksum = 0;
    if (!readRomIdentity(romPath, gameCode, checksum))
    {
        appendStubLog("GBAStationNDSStub: usrcheat rom identity failed rom=%s ms=%lld",
                      romPath.c_str(),
                      elapsedMs(loadBegin));
        return result;
    }

    std::ifstream file(result.sourcePath, std::ios::binary);
    std::uint64_t fileSize = 0;
    std::array<std::uint8_t, 16> header {};
    if (!file ||
        !getFileSize(file, fileSize) ||
        fileSize < 0x110 ||
        !seekTo(file, 0))
    {
        appendStubLog("GBAStationNDSStub: usrcheat invalid path=%s ms=%lld",
                      result.sourcePath.c_str(),
                      elapsedMs(loadBegin));
        return result;
    }
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (!file || std::memcmp(header.data(), "R4 CheatCode", 12) != 0)
    {
        appendStubLog("GBAStationNDSStub: usrcheat invalid header path=%s ms=%lld",
                      result.sourcePath.c_str(),
                      elapsedMs(loadBegin));
        return result;
    }
    const std::uint32_t version = readLe32(header.data() + 12);

    if (!seekTo(file, kDatHeaderSize))
        return result;

    std::vector<DatEntryInfo> matches;
    while (true)
    {
        std::array<std::uint8_t, 16> entry {};
        file.read(reinterpret_cast<char*>(entry.data()), static_cast<std::streamsize>(entry.size()));
        if (!file)
            break;

        DatEntryInfo info;
        info.gameCode = readLe32(entry.data());
        info.checksum = readLe32(entry.data() + 4);
        info.offset = readLe32(entry.data() + 8);
        if (info.gameCode == 0)
            break;
        if (info.gameCode == gameCode &&
            info.offset >= kDatHeaderSize &&
            info.offset < fileSize)
            matches.push_back(info);
    }

    if (matches.empty())
    {
        appendStubLog("GBAStationNDSStub: usrcheat no game match gameCode=%08X checksum=%08X path=%s ms=%lld",
                      gameCode,
                      checksum,
                      result.sourcePath.c_str(),
                      elapsedMs(loadBegin));
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
        if (parseCheatsAtOffset(file, fileSize, *candidate, parsed))
        {
            selected = candidate;
            result.gameName = std::move(parsed.gameName);
            result.items = std::move(parsed.items);
            result.skippedInvalidCodes = parsed.skippedInvalidCodes;
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

    appendStubLog("GBAStationNDSStub: usrcheat load path=%s version=%08X gameCode=%08X checksum=%08X entryChecksum=%08X entryOffset=%08X matched=%d items=%d skipped=%d game=%s ms=%lld",
                  result.sourcePath.c_str(),
                  version,
                  gameCode,
                  checksum,
                  selected->checksum,
                  selected->offset,
                  result.gameMatched ? 1 : 0,
                  static_cast<int>(result.items.size()),
                  result.skippedInvalidCodes,
                  result.gameName.c_str(),
                  elapsedMs(loadBegin));
    return result;
}

} // namespace beiklive::nds_stub
