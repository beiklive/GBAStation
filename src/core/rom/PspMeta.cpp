#include "PspMeta.hpp"

#include "miniz.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive
{
namespace psp_meta
{
namespace
{

// ── 小工具 ────────────────────────────────────────────────────────────────

uint32_t ReadU32LE(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
uint16_t ReadU16LE(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

// ISO9660 目录记录中的 7 字节 both-endian 值，实际内容是大端。
uint32_t ReadU32BE7(const uint8_t* p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

bool StartsWithCI(const std::string& s, const char* prefix)
{
    const size_t n = std::strlen(prefix);
    if (s.size() < n)
        return false;
    for (size_t i = 0; i < n; ++i)
    {
        char a = s[i];
        if (a >= 'A' && a <= 'Z')
            a = static_cast<char>(a - 'A' + 'a');
        char b = prefix[i];
        if (b >= 'A' && b <= 'Z')
            b = static_cast<char>(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}

std::string LowerCI(const std::string& s)
{
    std::string out = s;
    for (char& c : out)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return out;
}

bool EndsWith(const std::string& s, const char* suffix)
{
    const size_t n = std::strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

// 从 UTF-16LE 字节串转 UTF-8（SFO 0x0406 类型）。
std::string Utf16LeToUtf8(const uint8_t* data, size_t len)
{
    std::string out;
    out.reserve(len / 2);
    for (size_t i = 0; i + 1 < len; i += 2)
    {
        uint32_t cp = data[i] | (data[i + 1] << 8);
        if (cp == 0)
            break;
        if (cp < 0x80)
        {
            out.push_back(static_cast<char>(cp));
        }
        else if (cp < 0x800)
        {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

std::string TrimString(const std::string& s)
{
    size_t b = 0, e = s.size();
    while (b < e && static_cast<unsigned char>(s[b]) <= 0x20)
        ++b;
    while (e > b && static_cast<unsigned char>(s[e - 1]) <= 0x20)
        --e;
    return s.substr(b, e - b);
}

// ── SFO (PSF) 解析 ─────────────────────────────────────────────────────────

// 从 PARAM.SFO 字节中取指定 key 的字符串值。
bool SfoGetString(const std::vector<uint8_t>& sfo, const char* key, std::string& out)
{
    if (sfo.size() < 20)
        return false;
    if (sfo[0] != 'P' || sfo[1] != 'S' || sfo[2] != 'F')
        return false;

    const uint32_t keyTable = ReadU32LE(sfo.data() + 12);
    const uint32_t dataTable = ReadU32LE(sfo.data() + 16);
    const uint32_t count = ReadU32LE(sfo.data() + 20);
    if (keyTable + 20 * count > sfo.size() || keyTable < 20)
        return false;

    for (uint32_t i = 0; i < count; ++i)
    {
        const uint8_t* entry = sfo.data() + keyTable + i * 20;
        const uint32_t keyOffset = ReadU32LE(entry);
        const uint32_t fmt = ReadU32LE(entry + 4);
        const uint32_t dataLen = ReadU32LE(entry + 8);
        const uint32_t dataOffset = ReadU32LE(entry + 16);
        const size_t keyPos = keyTable + keyOffset;
        if (keyPos + 20 > sfo.size())
            continue;
        const char* k = reinterpret_cast<const char*>(sfo.data() + keyPos);
        if (std::strcmp(k, key) != 0)
            continue;
        if (dataTable + dataOffset + dataLen > sfo.size())
            return false;
        const uint8_t* d = sfo.data() + dataTable + dataOffset;
        if (fmt == 0x0406)
            out = Utf16LeToUtf8(d, dataLen);
        else
            out.assign(reinterpret_cast<const char*>(d), dataLen);
        return true;
    }
    return false;
}

// ── PBP 解析 ───────────────────────────────────────────────────────────────

enum PbpSubFile
{
    PBP_PARAM_SFO = 0,
    PBP_ICON0_PNG = 1,
    PBP_COUNT = 8,
};

// 从 EBOOT.PBP 字节中取子文件（PARAM_SFO / ICON0_PNG）。
bool PbpExtract(const std::vector<uint8_t>& pbp, PbpSubFile which, std::vector<uint8_t>& out)
{
    if (pbp.size() < 4 + 4 + 8 * 4)
        return false;
    if (!(pbp[0] == 0x00 && pbp[1] == 'P' && pbp[2] == 'B' && pbp[3] == 'P'))
        return false;

    const uint32_t start = ReadU32LE(pbp.data() + 8 + which * 4);
    const uint32_t end = ReadU32LE(pbp.data() + 8 + (which + 1) * 4);
    if (start >= end || end > pbp.size())
        return false;
    out.assign(pbp.begin() + start, pbp.begin() + end);
    return true;
}

// ── ISO9660 读取器 ─────────────────────────────────────────────────────────

class IsoReader
{
public:
    // data = 原始 ISO 镜像字节（解压 CSO 后或直接读文件）。
    explicit IsoReader(const std::vector<uint8_t>& data)
        : data_(data)
    {
    }

    bool Init()
    {
        // 第 16 扇区为 PVD。
        const size_t pvdOffset = 16 * 2048;
        if (pvdOffset + 2048 > data_.size())
            return false;
        const uint8_t* pvd = data_.data() + pvdOffset;
        if (pvd[0] != 1 || pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0' || pvd[4] != '0' || pvd[5] != '1')
            return false;
        // PVD +156：根目录记录。
        const uint8_t* rootRec = pvd + 156;
        rootLba_ = ReadU32BE7(rootRec + 2);
        rootSize_ = ReadU32BE7(rootRec + 10);
        return true;
    }

    // 读取路径（如 "PSP_GAME/PARAM.SFO"，忽略大小写）内容。
    bool ReadFile(const std::string& path, std::vector<uint8_t>& out)
    {
        if (rootSize_ == 0)
            return false;
        const uint8_t* rootDir = ReadDir(rootLba_, rootSize_);
        if (!rootDir)
            return false;

        std::vector<std::string> parts;
        std::string cur;
        for (char c : path)
        {
            if (c == '/' || c == '\\')
            {
                if (!cur.empty())
                    parts.push_back(LowerCI(cur));
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }
        if (!cur.empty())
            parts.push_back(LowerCI(cur));
        if (parts.empty())
            return false;

        const uint8_t* dir = rootDir;
        size_t dirSize = rootSize_;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            const bool last = (i + 1 == parts.size());
            uint32_t subLba = 0;
            const uint8_t* rec = FindEntry(dir, dirSize, parts[i], &subLba);
            if (!rec)
                return false;
            if (!last)
            {
                dir = ReadDir(subLba, recSize_);
                if (!dir)
                    return false;
                dirSize = recSize_;
                continue;
            }
            const uint32_t lba = ReadU32BE7(rec + 2);
            const uint32_t size = ReadU32BE7(rec + 10);
            const size_t off = static_cast<size_t>(lba) * 2048;
            if (off + size > data_.size())
                return false;
            out.assign(data_.begin() + off, data_.begin() + off + size);
            return true;
        }
        return false;
    }

private:
    // 读取目录扇区（可能跨块）。
    std::vector<uint8_t> ReadDirBytes(uint32_t lba, uint32_t size)
    {
        std::vector<uint8_t> buf;
        const size_t start = static_cast<size_t>(lba) * 2048;
        if (start + size > data_.size())
            return buf;
        buf.assign(data_.begin() + start, data_.begin() + start + size);
        return buf;
    }

    // 目录内容的稳定视图（需要生命周期管理，用 vector 拷贝）。
    const uint8_t* ReadDir(uint32_t lba, uint32_t size)
    {
        dirCache_ = ReadDirBytes(lba, size);
        return dirCache_.empty() ? nullptr : dirCache_.data();
    }

    // 在目录字节中查找名为 name 的条目。若 wantSub 输出其 lba/size。
    const uint8_t* FindEntry(const uint8_t* dir, size_t dirSize, const std::string& name,
                             uint32_t* subLba)
    {
        size_t pos = 0;
        while (pos + 34 <= dirSize)
        {
            const uint8_t* rec = dir + pos;
            const uint8_t len = rec[0];
            if (len == 0)
            {
                // 对齐填充：跳到下一个扇区边界。
                pos = (pos / 2048 + 1) * 2048;
                continue;
            }
            if (pos + len > dirSize)
                break;
            const uint8_t nameLen = rec[32];
            std::string entryName(reinterpret_cast<const char*>(rec + 33), nameLen);
            // ISO9660 文件名带 ";1" 版本后缀，目录项也可能没有。
            if (nameLen >= 2 && EndsWith(entryName, ";1"))
                entryName.resize(nameLen - 2);
            const std::string lower = LowerCI(entryName);
            if (lower == name)
            {
                if (subLba)
                    *subLba = ReadU32BE7(rec + 2);
                recSize_ = ReadU32BE7(rec + 10);
                return rec;
            }
            pos += len;
        }
        return nullptr;
    }

    const std::vector<uint8_t>& data_;
    uint32_t rootLba_ = 0;
    uint32_t rootSize_ = 0;
    uint32_t recSize_ = 0;
    std::vector<uint8_t> dirCache_;
};

// 将文件读入内存（上限保护：ISO 一般 < 2GB，这里限制 1GB 内存镜像？NO——
// CSO 解压需要全镜像，ISO 直接 mmap 不现实，改为按需读。此处用简单全读，
// PSP ISO 镜像通常 200MB-1.5GB，内存可承受；但为了稳妥限制为 2GB。
std::vector<uint8_t> ReadWholeFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return {};
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size <= 0 || size > 2LL * 1024 * 1024 * 1024)
        return {};
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(buf.data()), size);
    if (!in)
        return {};
    return buf;
}

// ── CSO (CISO) 解压 ───────────────────────────────────────────────────────

// 解压 CSO 为完整 ISO 镜像字节。成功返回 true，out 为镜像。
bool DecompressCso(const std::vector<uint8_t>& cso, std::vector<uint8_t>& out)
{
    if (cso.size() < 24)
        return false;
    if (!(cso[0] == 'C' && cso[1] == 'I' && cso[2] == 'S' && cso[3] == 'O'))
        return false;

    const uint32_t headerSize = ReadU32LE(cso.data() + 4);
    const uint64_t totalBytes = ReadU32LE(cso.data() + 8) |
                                (static_cast<uint64_t>(ReadU32LE(cso.data() + 12)) << 32);
    const uint32_t blockSize = ReadU32LE(cso.data() + 16);
    const uint8_t version = cso[20];
    const uint8_t align = cso[21];
    if (headerSize < 24 || totalBytes == 0 || blockSize == 0 ||
        (version != 1 && version != 2))
        return false;
    const size_t shift = align; // 块偏移按 align 字节对齐
    const uint32_t totalBlocks = static_cast<uint32_t>((totalBytes + blockSize - 1) / blockSize);
    if (headerSize + static_cast<size_t>(totalBlocks) * 4 > cso.size())
        return false;

    out.clear();
    out.reserve(static_cast<size_t>(totalBytes));
    for (uint32_t b = 0; b < totalBlocks; ++b)
    {
        const uint32_t idx = ReadU32LE(cso.data() + headerSize + b * 4);
        const uint32_t nextIdx = ReadU32LE(cso.data() + headerSize + (b + 1) * 4);
        const bool compressed = (idx >> 31) == 0;
        const size_t start = (static_cast<size_t>(idx & 0x7FFFFFFF)) << shift;
        const size_t end = (static_cast<size_t>(nextIdx & 0x7FFFFFFF)) << shift;
        if (start > cso.size() || end > cso.size() || end < start)
            return false;
        const size_t packedSize = end - start;
        const size_t want = (b + 1) * static_cast<size_t>(blockSize) <= totalBytes
                                ? blockSize
                                : static_cast<size_t>(totalBytes) - b * blockSize;
        std::vector<uint8_t> block(want);
        if (compressed)
        {
            mz_ulong outLen = static_cast<mz_ulong>(want);
            const int rc = mz_uncompress(block.data(), &outLen, cso.data() + start,
                                         static_cast<mz_ulong>(packedSize));
            if (rc != MZ_OK || outLen != want)
                return false;
        }
        else
        {
            if (packedSize < want)
                return false;
            std::memcpy(block.data(), cso.data() + start, want);
        }
        out.insert(out.end(), block.begin(), block.end());
    }
    return true;
}

// ── 顶层流程 ──────────────────────────────────────────────────────────────

bool IsLikelyPbp(const std::vector<uint8_t>& head)
{
    return head.size() >= 4 && head[0] == 0x00 && head[1] == 'P' && head[2] == 'B' && head[3] == 'P';
}

// 从 ISO 镜像字节解析（已解压的完整镜像）。
bool ExtractFromIsoBytes(const std::vector<uint8_t>& iso, std::vector<uint8_t>* sfo,
                         std::vector<uint8_t>* icon0)
{
    IsoReader reader(iso);
    if (!reader.Init())
        return false;
    bool any = false;
    if (sfo)
    {
        any = reader.ReadFile("PSP_GAME/PARAM.SFO", *sfo) || any;
    }
    if (icon0)
    {
        any = reader.ReadFile("PSP_GAME/ICON0.PNG", *icon0) || any;
    }
    return any;
}

// 打开文件：PBP 直接读子文件；ISO/CSO 解压后走 ISO9660；目录走 PSP_GAME。
// 返回 true 表示找到至少一个请求的数据。
bool ExtractFromPath(const std::string& path, std::vector<uint8_t>* sfo, std::vector<uint8_t>* icon0)
{
    fs::path p(path);
    if (fs::is_directory(p))
    {
        const fs::path gameDir = p / "PSP_GAME";
        bool any = false;
        if (sfo)
        {
            std::ifstream in(gameDir / "PARAM.SFO", std::ios::binary);
            if (in)
            {
                sfo->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
                any = true;
            }
        }
        if (icon0)
        {
            std::ifstream in(gameDir / "ICON0.PNG", std::ios::binary);
            if (in)
            {
                icon0->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
                any = true;
            }
        }
        return any;
    }

    std::ifstream headIn(path, std::ios::binary);
    std::array<char, 8> head{};
    if (!headIn || !headIn.read(head.data(), 8))
        return false;
    headIn.close();

    if (IsLikelyPbp(std::vector<uint8_t>(head.begin(), head.end())))
    {
        const std::vector<uint8_t> pbp = ReadWholeFile(path);
        if (pbp.empty())
            return false;
        bool any = false;
        if (sfo)
        {
            if (PbpExtract(pbp, PBP_PARAM_SFO, *sfo))
                any = true;
        }
        if (icon0)
        {
            if (PbpExtract(pbp, PBP_ICON0_PNG, *icon0))
                any = true;
        }
        return any;
    }

    const bool isCso = head[0] == 'C' && head[1] == 'I' && head[2] == 'S' && head[3] == 'O';
    std::vector<uint8_t> iso;
    if (isCso)
    {
        const std::vector<uint8_t> cso = ReadWholeFile(path);
        if (cso.empty() || !DecompressCso(cso, iso))
            return false;
    }
    else
    {
        // 普通 ISO：只读所需的扇区区域即可，但简单起见全读（由大小上限保护）。
        iso = ReadWholeFile(path);
        if (iso.empty())
            return false;
    }
    return ExtractFromIsoBytes(iso, sfo, icon0);
}

std::string SanitizeStem(const std::string& stem)
{
    std::string out;
    for (char c : stem)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            out.push_back('_');
        else
            out.push_back(c);
    }
    return out.empty() ? "game" : out;
}

} // namespace

// ── 公开 API ───────────────────────────────────────────────────────────────

std::string ExtractTitle(const std::string& path)
{
    std::vector<uint8_t> sfo;
    if (!ExtractFromPath(path, &sfo, nullptr) || sfo.empty())
        return {};
    std::string title;
    if (!SfoGetString(sfo, "TITLE", title))
        return {};
    return TrimString(title);
}

std::string ExtractIcon0(const std::string& path, const std::string& cacheDir)
{
    std::vector<uint8_t> icon0;
    if (!ExtractFromPath(path, nullptr, &icon0) || icon0.empty())
        return {};

    // PNG 校验头。
    if (icon0.size() < 8)
        return {};
    if (!(icon0[0] == 0x89 && icon0[1] == 'P' && icon0[2] == 'N' && icon0[3] == 'G'))
        return {};

    std::error_code ec;
    fs::create_directories(cacheDir, ec);
    const std::string stem = SanitizeStem(fs::path(path).stem().string());
    const fs::path outPath = fs::path(cacheDir) / (stem + ".icon0.png");
    std::ofstream out(outPath, std::ios::binary);
    if (!out)
        return {};
    out.write(reinterpret_cast<const char*>(icon0.data()), static_cast<std::streamsize>(icon0.size()));
    if (!out)
        return {};
    return outPath.string();
}

} // namespace psp_meta
} // namespace beiklive
