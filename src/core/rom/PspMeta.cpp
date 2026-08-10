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

// ISO9660 目录记录中的 8 字节 both-endian 值（前 4 字节大端，后 4 字节小端）。
uint32_t ReadU32BE7(const uint8_t* p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

// both-endian 字段兼容读取：PSP UMD 镜像的目录记录字段是 [LE][BE]（小端在前），
// 标准 ISO9660 是 [BE][LE]。取两者中数值合理（指向文件范围内）的那个。
// LBA（扇区号）用 limit=文件字节数 校验；返回扇区号。
uint32_t ReadU32BothEndianLba(const uint8_t* p, uint64_t fileSize)
{
    const uint32_t le = ReadU32LE(p);
    const uint32_t be = ReadU32BE7(p);
    if (le > 0 && static_cast<uint64_t>(le) * 2048 < fileSize)
        return le;
    return be;
}

// both-endian 字段兼容读取：数据长度（字节）版本。
uint32_t ReadU32BothEndianSize(const uint8_t* p, uint64_t fileSize)
{
    const uint32_t le = ReadU32LE(p);
    const uint32_t be = ReadU32BE7(p);
    if (le > 0 && static_cast<uint64_t>(le) <= fileSize)
        return le;
    return be;
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

// ── 按需读取抽象 ──────────────────────────────────────────────────────────
// ISO/CSO 统一通过 ByteRangeReader 按字节范围读取，不再把整个镜像读入内存。
class ByteRangeReader
{
public:
    virtual ~ByteRangeReader() = default;
    virtual bool Read(uint64_t offset, size_t length, std::vector<uint8_t>& out) const = 0;
    virtual uint64_t Size() const = 0;
};

class FileByteReader final : public ByteRangeReader
{
public:
    explicit FileByteReader(std::ifstream& file, uint64_t size)
        : file_(&file), size_(size)
    {
    }

    bool Read(uint64_t offset, size_t length, std::vector<uint8_t>& out) const override
    {
        if (offset + length > size_)
            return false;
        out.resize(length);
        file_->clear();
        file_->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!file_->read(reinterpret_cast<char*>(out.data()),
                         static_cast<std::streamsize>(length)))
            return false;
        return true;
    }

    uint64_t Size() const override { return size_; }

private:
    std::ifstream* file_;
    uint64_t size_;
};

// ── CSO (CISO) 按块解压读取 ───────────────────────────────────────────────
class CsoByteReader final : public ByteRangeReader
{
public:
    explicit CsoByteReader(std::ifstream& file)
        : file_(&file)
    {
    }

    bool Init()
    {
        std::vector<uint8_t> hdr(24);
        if (!ReadAt(0, hdr))
            return false;
        if (!(hdr[0] == 0x43 && hdr[1] == 0x49 && hdr[2] == 0x53 && hdr[3] == 0x4F))
            return false;
        const uint32_t headerSize = ReadU32LE(hdr.data() + 4);
        totalBytes_ = ReadU32LE(hdr.data() + 8) |
                      (static_cast<uint64_t>(ReadU32LE(hdr.data() + 12)) << 32);
        blockSize_ = ReadU32LE(hdr.data() + 16);
        const uint8_t version = hdr[20];
        const uint8_t align = hdr[21];
        if (headerSize < 24 || totalBytes_ == 0 || blockSize_ == 0 ||
            (version != 1 && version != 2))
            return false;
        shift_ = align; // 块偏移按 align 字节对齐
        totalBlocks_ = static_cast<uint32_t>((totalBytes_ + blockSize_ - 1) / blockSize_);
        const uint64_t indexBytes = static_cast<uint64_t>(totalBlocks_ + 1) * 4;
        if (static_cast<uint64_t>(headerSize) + indexBytes > Size())
            return false;
        std::vector<uint8_t> indices(static_cast<size_t>(indexBytes));
        if (!ReadAt(headerSize, indices))
            return false;
        indexData_.resize(totalBlocks_ + 1);
        for (uint32_t b = 0; b <= totalBlocks_; ++b)
            indexData_[b] = ReadU32LE(indices.data() + static_cast<size_t>(b) * 4);
        return true;
    }

    bool Read(uint64_t offset, size_t length, std::vector<uint8_t>& out) const override
    {
        if (offset + length > Size())
            return false;
        out.assign(length, 0);
        size_t done = 0;
        while (done < length)
        {
            const uint64_t pos = offset + done;
            const uint32_t block = static_cast<uint32_t>(pos / blockSize_);
            if (block >= totalBlocks_)
                return false;
            std::vector<uint8_t> blockData;
            if (!ReadBlock(block, blockData))
                return false;
            const size_t inBlock = static_cast<size_t>(pos % blockSize_);
            if (inBlock >= blockData.size())
                return false;
            const size_t copyLen = std::min(blockData.size() - inBlock, length - done);
            std::memcpy(out.data() + done, blockData.data() + inBlock, copyLen);
            done += copyLen;
        }
        return true;
    }

    uint64_t Size() const override
    {
        file_->clear();
        return static_cast<uint64_t>(file_->seekg(0, std::ios::end).tellg());
    }

private:
    bool ReadAt(uint64_t offset, std::vector<uint8_t>& out) const
    {
        if (out.empty())
            return true;
        file_->clear();
        file_->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        file_->read(reinterpret_cast<char*>(out.data()),
                   static_cast<std::streamsize>(out.size()));
        return file_->gcount() == static_cast<std::streamsize>(out.size());
    }

    bool ReadBlock(uint32_t block, std::vector<uint8_t>& out) const
    {
        const uint32_t idx = indexData_[block];
        const uint32_t nextIdx = indexData_[block + 1];
        const bool compressed = (idx >> 31) == 0;
        const size_t start = (static_cast<size_t>(idx & 0x7FFFFFFF)) << shift_;
        const size_t end = (static_cast<size_t>(nextIdx & 0x7FFFFFFF)) << shift_;
        if (start > end || end > Size())
            return false;
        const uint64_t blockStart = static_cast<uint64_t>(block) * blockSize_;
        const size_t blockLen = static_cast<size_t>(
            std::min<uint64_t>(blockSize_, totalBytes_ - blockStart));
        if (compressed)
        {
            const size_t packedSize = end - start;
            std::vector<uint8_t> packed(packedSize);
            if (!ReadAt(start, packed))
                return false;
            out.resize(blockLen);
            mz_ulong outLen = static_cast<mz_ulong>(blockLen);
            const int rc = mz_uncompress(out.data(), &outLen, packed.data(),
                                         static_cast<mz_ulong>(packedSize));
            if (rc != MZ_OK || outLen != blockLen)
                return false;
        }
        else
        {
            if (end - start < blockLen)
                return false;
            out.resize(blockLen);
            if (!ReadAt(start, out))
                return false;
        }
        return true;
    }

    std::ifstream* file_;
    uint64_t totalBytes_ = 0;
    uint32_t blockSize_ = 0;
    uint32_t totalBlocks_ = 0;
    uint32_t shift_ = 0;
    std::vector<uint32_t> indexData_;
};

// 将 PBP 读入内存（体积小，上限 2GB）。
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

// ── ISO9660 流式读取器 ────────────────────────────────────────────────────
class IsoReader
{
public:
    explicit IsoReader(const ByteRangeReader* reader)
        : reader_(reader)
    {
    }

    bool Init()
    {
        // 第 16 扇区为 PVD。
        std::vector<uint8_t> pvd;
        if (!reader_->Read(16ULL * 2048, 2048, pvd) || pvd.size() < 2048)
            return false;
        const uint8_t* p = pvd.data();
        if (p[0] != 1 || p[1] != 0x43 || p[2] != 0x44 || p[3] != 0x30 || p[4] != 0x30 || p[5] != 0x31)
            return false;
        // PVD +156：根目录记录。
        const uint8_t* rootRec = p + 156;
        rootLba_ = ReadU32BothEndianLba(rootRec + 2, reader_->Size());
        rootSize_ = ReadU32BothEndianSize(rootRec + 10, reader_->Size());
        return true;
    }

    // 读取路径（如 "PSP_GAME/PARAM.SFO"，忽略大小写）内容。
    bool ReadFile(const std::string& path, std::vector<uint8_t>& out)
    {
        if (rootSize_ == 0)
            return false;
        std::vector<uint8_t> dir;
        if (!ReadDir(rootLba_, rootSize_, dir))
            return false;

        std::vector<std::string> parts;
        std::string cur;
        for (char c : path)
        {
            if (c == 0x2F || c == 0x5C)
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

        for (size_t i = 0; i < parts.size(); ++i)
        {
            const bool last = (i + 1 == parts.size());
            uint32_t subLba = 0;
            uint32_t subSize = 0;
            const uint8_t* rec = FindEntry(dir, parts[i], &subLba, &subSize);
            if (!rec)
                return false;
            if (!last)
            {
                if (!ReadDir(subLba, subSize, dir))
                    return false;
                continue;
            }
            const uint32_t lba = ReadU32BothEndianLba(rec + 2, reader_->Size());
            const uint32_t size = ReadU32BothEndianSize(rec + 10, reader_->Size());
            return reader_->Read(static_cast<uint64_t>(lba) * 2048, size, out);
        }
        return false;
    }

private:
    bool ReadDir(uint32_t lba, uint32_t size, std::vector<uint8_t>& out)
    {
        return reader_->Read(static_cast<uint64_t>(lba) * 2048, size, out);
    }

    // 在目录字节中查找名为 name 的条目，输出其 lba/size。
    const uint8_t* FindEntry(const std::vector<uint8_t>& dir, const std::string& name,
                             uint32_t* subLba, uint32_t* subSize)
    {
        const size_t dirSize = dir.size();
        size_t pos = 0;
        while (pos + 34 <= dirSize)
        {
            const uint8_t* rec = dir.data() + pos;
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
            if (LowerCI(entryName) == name)
            {
                if (subLba)
                    *subLba = ReadU32BothEndianLba(rec + 2, reader_->Size());
                if (subSize)
                    *subSize = ReadU32BothEndianSize(rec + 10, reader_->Size());
                return rec;
            }
            pos += len;
        }
        return nullptr;
    }

    const ByteRangeReader* reader_;
    uint32_t rootLba_ = 0;
    uint32_t rootSize_ = 0;
};
bool IsLikelyPbp(const std::vector<uint8_t>& head)
{
    return head.size() >= 4 && head[0] == 0x00 && head[1] == 'P' && head[2] == 'B' && head[3] == 'P';
}

// 打开文件：PBP 直接读子文件；ISO/CSO 按需流式读取；目录走 PSP_GAME。
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

    std::ifstream file(path, std::ios::binary);
    std::array<char, 8> head{};
    if (!file || !file.read(head.data(), 8))
        return false;
    file.seekg(0, std::ios::end);
    const std::streamoff fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize < 8)
        return false;

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

    // ISO / CSO：按需读取（只读 PVD、目录链与目标文件扇区，不再全镜像读入）。
    std::unique_ptr<ByteRangeReader> reader;
    const bool isCso = head[0] == 0x43 && head[1] == 0x49 && head[2] == 0x53 && head[3] == 0x4F;
    if (isCso)
    {
        auto cso = std::make_unique<CsoByteReader>(file);
        if (!cso->Init())
            return false;
        reader = std::move(cso);
    }
    else
    {
        reader = std::make_unique<FileByteReader>(file, static_cast<uint64_t>(fileSize));
    }

    IsoReader iso(reader.get());
    if (!iso.Init())
        return false;
    bool any = false;
    if (sfo)
    {
        if (iso.ReadFile("PSP_GAME/PARAM.SFO", *sfo))
            any = true;
    }
    if (icon0)
    {
        if (iso.ReadFile("PSP_GAME/ICON0.PNG", *icon0))
            any = true;
    }
    return any;
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
