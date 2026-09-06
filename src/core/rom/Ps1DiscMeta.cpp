#include "Ps1DiscMeta.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive::ps1_meta
{
namespace
{

uint32_t readLe32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

uint32_t readBe32(const uint8_t* p)
{
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) |
           static_cast<uint32_t>(p[3]);
}

uint32_t readIsoBothEndian(const uint8_t* p)
{
    const uint32_t le = readLe32(p);
    const uint32_t be = readBe32(p + 4);
    return le != 0 ? le : be;
}

std::string trim(std::string value)
{
    const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char c) {
        return !isSpace(static_cast<unsigned char>(c));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char c) {
        return !isSpace(static_cast<unsigned char>(c));
    }).base(), value.end());
    return value;
}

std::string upper(std::string value)
{
    for (char& c : value)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

bool readAt(std::ifstream& file, uint64_t offset, size_t size, std::vector<uint8_t>& out)
{
    file.clear();
    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!file)
        return false;
    out.resize(size);
    return static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()),
                                       static_cast<std::streamsize>(size)));
}

class Reader
{
public:
    Reader(std::ifstream& file, uint64_t size, uint32_t sectorSize, uint32_t dataOffset)
        : file_(file), size_(size), sectorSize_(sectorSize), dataOffset_(dataOffset)
    {
    }

    bool readUserData(uint32_t lba, size_t size, std::vector<uint8_t>& out)
    {
        const uint64_t offset = static_cast<uint64_t>(lba) * sectorSize_ + dataOffset_;
        if (offset + size > size_)
            return false;
        return readAt(file_, offset, size, out);
    }

    uint32_t sectorSize() const { return sectorSize_; }

private:
    std::ifstream& file_;
    uint64_t size_;
    uint32_t sectorSize_;
    uint32_t dataOffset_;
};

bool isPvd(const std::vector<uint8_t>& pvd)
{
    return pvd.size() >= 7 && pvd[0] == 1 && pvd[1] == 'C' && pvd[2] == 'D' &&
           pvd[3] == '0' && pvd[4] == '0' && pvd[5] == '1';
}

bool locateSystemCnf(Reader& reader, uint32_t& lba, uint32_t& length)
{
    std::vector<uint8_t> pvd;
    if (!reader.readUserData(16, 2048, pvd) || !isPvd(pvd))
        return false;
    const uint8_t* root = pvd.data() + 156;
    if (root[0] < 34)
        return false;
    const uint32_t rootLba = readIsoBothEndian(root + 2);
    const uint32_t rootSize = readIsoBothEndian(root + 10);
    std::vector<uint8_t> directory;
    if (!reader.readUserData(rootLba, rootSize, directory))
        return false;

    for (size_t pos = 0; pos + 34 <= directory.size();)
    {
        const uint8_t recordLength = directory[pos];
        if (recordLength == 0)
        {
            pos = ((pos / reader.sectorSize()) + 1) * reader.sectorSize();
            continue;
        }
        if (pos + recordLength > directory.size() || recordLength < 34)
            break;
        const uint8_t nameLength = directory[pos + 32];
        if (33U + nameLength <= recordLength)
        {
            std::string name(reinterpret_cast<const char*>(directory.data() + pos + 33), nameLength);
            if (name.size() >= 2 && name.compare(name.size() - 2, 2, ";1") == 0)
                name.resize(name.size() - 2);
            if (upper(name) == "SYSTEM.CNF")
            {
                lba = readIsoBothEndian(directory.data() + pos + 2);
                length = readIsoBothEndian(directory.data() + pos + 10);
                return true;
            }
        }
        pos += recordLength;
    }
    return false;
}

std::string parseBoot(const std::vector<uint8_t>& data)
{
    std::string text(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line))
    {
        line = trim(line);
        const size_t equal = line.find('=');
        if (equal == std::string::npos)
            continue;
        const std::string key = upper(trim(line.substr(0, equal)));
        if (key == "BOOT")
            return trim(line.substr(equal + 1));
    }
    return {};
}

bool parseCue(const std::string& cuePath, fs::path& dataPath, uint32_t& sectorSize, uint32_t& dataOffset)
{
    std::ifstream cue(cuePath);
    if (!cue)
        return false;
    fs::path currentFile;
    std::string line;
    while (std::getline(cue, line))
    {
        const std::string t = trim(line);
        if (t.size() >= 4 && upper(t.substr(0, 4)) == "FILE")
        {
            const size_t first = t.find('"');
            const size_t second = first == std::string::npos ? std::string::npos : t.find('"', first + 1);
            if (first != std::string::npos && second != std::string::npos)
                currentFile = fs::path(t.substr(first + 1, second - first - 1));
            else
            {
                std::istringstream stream(t.substr(4));
                stream >> currentFile;
            }
        }
        const size_t track = upper(t).find("TRACK");
        if (track != std::string::npos && upper(t).find("MODE1/2048") != std::string::npos)
        {
            dataPath = fs::path(cuePath).parent_path() / currentFile;
            sectorSize = 2048;
            dataOffset = 0;
            return true;
        }
        if (track != std::string::npos && upper(t).find("MODE1/2352") != std::string::npos)
        {
            dataPath = fs::path(cuePath).parent_path() / currentFile;
            sectorSize = 2352;
            dataOffset = 16;
            return true;
        }
    }
    return false;
}

std::string extractFromFile(const fs::path& filePath, uint32_t sectorSize, uint32_t dataOffset)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        return {};
    file.seekg(0, std::ios::end);
    const std::streamoff end = file.tellg();
    if (end <= 0)
        return {};
    Reader reader(file, static_cast<uint64_t>(end), sectorSize, dataOffset);
    std::vector<uint8_t> pvd;
    if (!reader.readUserData(16, 2048, pvd) || !isPvd(pvd))
        return {};
    uint32_t lba = 0;
    uint32_t length = 0;
    if (!locateSystemCnf(reader, lba, length))
        return {};
    std::vector<uint8_t> cnf;
    if (!reader.readUserData(lba, length, cnf))
        return {};
    return NormalizeSerialFromBootPath(parseBoot(cnf));
}

} // namespace

std::string NormalizeSerialFromBootPath(std::string_view bootPath)
{
    std::string serial(bootPath);
    const size_t slash = serial.find_last_of("\\/");
    if (slash != std::string::npos)
        serial.erase(0, slash + 1);
    const size_t colon = serial.find_last_of(':');
    if (colon != std::string::npos)
        serial.erase(0, colon + 1);
    const size_t version = serial.find(';');
    if (version != std::string::npos)
        serial.erase(version);
    serial.erase(std::remove(serial.begin(), serial.end(), '.'), serial.end());
    std::replace(serial.begin(), serial.end(), '_', '-');
    return upper(serial);
}

std::string ExtractSerial(const std::string& path)
{
    const fs::path input(path);
    const std::string ext = upper(input.extension().string());
    if (ext == ".CUE")
    {
        fs::path dataPath;
        uint32_t sectorSize = 0;
        uint32_t dataOffset = 0;
        if (!parseCue(path, dataPath, sectorSize, dataOffset))
            return {};
        return extractFromFile(dataPath, sectorSize, dataOffset);
    }
    if (ext != ".ISO" && ext != ".BIN" && ext != ".IMG")
        return {};

    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size <= 16 * 2048)
        return {};
    file.close();

    // Raw MODE1/2048 images are most common. MODE1/2352 has a 16-byte sector header.
    if (const std::string serial = extractFromFile(path, 2048, 0); !serial.empty())
        return serial;
    return extractFromFile(path, 2352, 16);
}

} // namespace beiklive::ps1_meta
