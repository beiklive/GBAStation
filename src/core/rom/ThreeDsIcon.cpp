#include "ThreeDsIcon.hpp"

#include "../common.h"

#include "third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive
{
namespace
{

constexpr size_t kSmdhSize = 0x36C0;        // SMDH 文件大小
constexpr size_t kLargeIconOffset = 0x24C0; // SMDH 内 48x48 大图标（RGB565, Morton）
constexpr size_t kLargeIconSize = 0x1200;
constexpr int kIconSize = 48;

uint32_t ReadU32LE(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint64_t ReadU64LE(const uint8_t* p)
{
    return static_cast<uint64_t>(ReadU32LE(p)) | (static_cast<uint64_t>(ReadU32LE(p + 4)) << 32);
}

size_t Align64(size_t value)
{
    return (value + 0x3F) & ~static_cast<size_t>(0x3F);
}

bool IsSmdh(const std::vector<uint8_t>& data)
{
    return data.size() >= 4 && data[0] == 'S' && data[1] == 'M' && data[2] == 'D' &&
           data[3] == 'H';
}

// 流式读取辅助：只读取指定区间的字节，避免整文件载入大容量 3DS ROM。
bool ReadRange(std::ifstream& in, std::streamoff offset, size_t length,
               std::vector<uint8_t>& out)
{
    out.resize(length);
    in.clear();
    in.seekg(offset, std::ios::beg);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
    return in.gcount() == static_cast<std::streamsize>(length);
}

bool IsNcchMagic(const std::vector<uint8_t>& head)
{
    return head.size() >= 0x104 && head[0x100] == 'N' && head[0x101] == 'C' &&
           head[0x102] == 'C' && head[0x103] == 'H';
}

// 从 NCCH 区域流式提取 SMDH（ExeFS 中的 icon.bin）。
// ncchStart 为该 NCCH 头在文件中的偏移，ncchHeader 为已读取的 0x200 字节头。
bool ExtractSmdhFromNcchStream(std::ifstream& in, std::streamoff ncchStart,
                               const std::vector<uint8_t>& ncchHeader,
                               std::vector<uint8_t>& smdh)
{
    if (!IsNcchMagic(ncchHeader))
        return false;

    const uint32_t exefsOff = ReadU32LE(ncchHeader.data() + 0x118); // media units
    const uint32_t exefsSize = ReadU32LE(ncchHeader.data() + 0x11C);
    if (exefsOff == 0 || exefsSize < 0x200)
        return false;
    const std::streamoff exefsStart =
        ncchStart + static_cast<std::streamoff>(exefsOff) * 0x200;

    std::vector<uint8_t> exefsHeader;
    if (!ReadRange(in, exefsStart, 0x200, exefsHeader))
        return false;

    for (int i = 0; i < 10; ++i)
    {
        const uint8_t* entry = exefsHeader.data() + static_cast<size_t>(i) * 0x20;
        if (std::memcmp(entry, "icon", 4) != 0)
            continue;
        const uint32_t fileOff = ReadU32LE(entry + 8);
        const uint32_t fileSize = ReadU32LE(entry + 12);
        if (fileSize < kSmdhSize)
            return false;
        std::vector<uint8_t> icon;
        if (!ReadRange(in, exefsStart + 0x200 + fileOff, kSmdhSize, icon))
            return false;
        smdh = std::move(icon);
        return IsSmdh(smdh);
    }
    return false;
}

// 从 NCSD（.3ds/.cci）流式提取 SMDH：分区 0 为 CXI（NCCH）。
bool ExtractSmdhFromNcsdStream(std::ifstream& in, const std::vector<uint8_t>& head,
                               std::vector<uint8_t>& smdh)
{
    if (!(head.size() >= 0x104 && head[0x100] == 'N' && head[0x101] == 'C' &&
          head[0x102] == 'S' && head[0x103] == 'D'))
        return false;

    const uint32_t partOff = ReadU32LE(head.data() + 0x140); // 分区 0 偏移（media units）
    if (partOff == 0)
        return false;
    const std::streamoff ncchStart = static_cast<std::streamoff>(partOff) * 0x200;

    std::vector<uint8_t> ncchHeader;
    if (!ReadRange(in, ncchStart, 0x200, ncchHeader))
        return false;
    return ExtractSmdhFromNcchStream(in, ncchStart, ncchHeader, smdh);
}

// 从 CIA 流式提取 SMDH：优先 meta chunk 内嵌的 SMDH，回退 content[0]（NCCH）。
bool ExtractSmdhFromCiaStream(std::ifstream& in, const std::vector<uint8_t>& head,
                              std::vector<uint8_t>& smdh)
{
    if (head.size() < 0x28)
        return false;
    const uint32_t headerSize = ReadU32LE(head.data() + 0x00);
    const uint32_t certSize = ReadU32LE(head.data() + 0x08);
    const uint32_t tikSize = ReadU32LE(head.data() + 0x0C);
    const uint32_t tmdSize = ReadU32LE(head.data() + 0x10);
    const uint32_t metaSize = ReadU32LE(head.data() + 0x14);
    const uint64_t contentSize = ReadU64LE(head.data() + 0x18);
    if (headerSize == 0 || headerSize > 0x2020)
        return false;

    const size_t certOff = Align64(headerSize);
    const size_t tikOff = Align64(certOff + certSize);
    const size_t tmdOff = Align64(tikOff + tikSize);
    const size_t contentOff = Align64(tmdOff + tmdSize);

    // meta chunk 位于所有 content 之后，内含 0x400 的 Metadata 与可选 SMDH。
    if (metaSize >= 0x400 + kSmdhSize)
    {
        const size_t metaOff = Align64(contentOff + static_cast<size_t>(contentSize));
        std::vector<uint8_t> meta;
        if (ReadRange(in, static_cast<std::streamoff>(metaOff + 0x400),
                      kSmdhSize, meta))
        {
            if (IsSmdh(meta))
            {
                smdh = std::move(meta);
                return true;
            }
        }
    }

    // 回退：从第一个 content（.app/NCCH）的 ExeFS 提取 icon.bin。
    std::vector<uint8_t> ncchHeader;
    if (!ReadRange(in, static_cast<std::streamoff>(contentOff), 0x200, ncchHeader))
        return false;
    return ExtractSmdhFromNcchStream(in, static_cast<std::streamoff>(contentOff),
                                     ncchHeader, smdh);
}

std::string LowerExt(const std::string& path)
{
    std::string ext = fs::path(path).extension().string();
    for (char& c : ext)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return ext;
}

bool ExtractSmdh(const std::string& path, std::vector<uint8_t>& smdh)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;

    // 只读取文件头部判定格式，正文按偏移流式读取（不再整文件载入）。
    std::vector<uint8_t> head;
    if (!ReadRange(in, 0, 0x200, head))
        return false;

    if (IsNcchMagic(head))
        return ExtractSmdhFromNcchStream(in, 0, head, smdh);
    if (head.size() >= 0x104 &&
        head[0x100] == 'N' && head[0x101] == 'C' && head[0x102] == 'S' && head[0x103] == 'D')
        return ExtractSmdhFromNcsdStream(in, head, smdh);
    if (LowerExt(path) == ".cia")
        return ExtractSmdhFromCiaStream(in, head, smdh);
    return false;
}

// SMDH 大图标（Morton/Z 顺序 RGB565）解码为 RGBA8888。
bool RenderSmdhIcon(const std::vector<uint8_t>& smdh, std::vector<uint8_t>& rgba)
{
    if (smdh.size() < kLargeIconOffset + kLargeIconSize || !IsSmdh(smdh))
        return false;

    static constexpr uint32_t kXlut[] = {0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15};
    static constexpr uint32_t kYlut[] = {0x00, 0x02, 0x08, 0x0a, 0x20, 0x22, 0x28, 0x2a};
    const uint8_t* icon = smdh.data() + kLargeIconOffset;
    rgba.assign(static_cast<size_t>(kIconSize) * kIconSize * 4, 0);

    for (int y = 0; y < kIconSize; ++y)
    {
        const uint32_t coarseY = static_cast<uint32_t>(y & ~7);
        for (int x = 0; x < kIconSize; ++x)
        {
            const uint32_t morton = kXlut[x & 7] + kYlut[y & 7];
            const size_t off =
                (static_cast<size_t>(morton) + static_cast<size_t>(x & ~7) * 8) * 2 +
                static_cast<size_t>(coarseY) * kIconSize * 2;
            const uint16_t color = static_cast<uint16_t>(icon[off] | (icon[off + 1] << 8));
            const uint8_t r = static_cast<uint8_t>((((color >> 11) & 0x1F) << 3) |
                                                   (((color >> 11) & 0x1F) >> 2));
            const uint8_t g = static_cast<uint8_t>((((color >> 5) & 0x3F) << 2) |
                                                   (((color >> 5) & 0x3F) >> 4));
            const uint8_t b = static_cast<uint8_t>(((color & 0x1F) << 3) |
                                                   ((color & 0x1F) >> 2));
            uint8_t* px = rgba.data() + (static_cast<size_t>(y) * kIconSize + x) * 4;
            px[0] = r;
            px[1] = g;
            px[2] = b;
            px[3] = 0xFF;
        }
    }
    return true;
}

// SMDH 标题（0x100 起 16 条 × 0x200，UTF-16LE；long title 在 +0x80）。
std::string SmdhTitle(const std::vector<uint8_t>& smdh, size_t langIndex)
{
    if (!IsSmdh(smdh) || smdh.size() < 0x100 + 16 * 0x200)
        return {};
    const uint8_t* p = smdh.data() + 0x100 + langIndex * 0x200 + 0x80; // long title
    std::string out;
    for (int i = 0; i < 0x100 / 2; ++i)
    {
        const uint16_t cp = static_cast<uint16_t>(p[i * 2] | (p[i * 2 + 1] << 8));
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
    std::string trimmed;
    size_t b = 0, e = out.size();
    while (b < e && static_cast<unsigned char>(out[b]) <= 0x20)
        ++b;
    while (e > b && static_cast<unsigned char>(out[e - 1]) <= 0x20)
        --e;
    trimmed = out.substr(b, e - b);
    return trimmed;
}

// FNV-1a 64 位哈希（与 NDS 图标缓存键同款）。
uint64_t Fnv1a64(const std::string& text)
{
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char c : text)
    {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string Hex64(uint64_t value)
{
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(16);
    for (int shift = 60; shift >= 0; shift -= 4)
        out.push_back(kDigits[(value >> shift) & 0xF]);
    return out;
}

} // namespace

std::string GetThreeDsIconCachePath(const std::string& romPath)
{
    if (romPath.empty())
        return "";

    std::error_code ec;
    fs::path romFsPath = fs::absolute(fs::path(romPath), ec);
    if (ec)
        romFsPath = fs::path(romPath);

    std::ostringstream key;
    key << romFsPath.lexically_normal().string();
    key << '|';
    const auto romSize = fs::file_size(romPath, ec);
    if (!ec)
        key << romSize;
    else
    {
        ec.clear();
        key << 0;
    }
    key << '|';
    const auto writeTime = fs::last_write_time(romPath, ec);
    if (!ec)
    {
        const auto writeTicks = std::chrono::duration_cast<std::chrono::nanoseconds>(
            writeTime.time_since_epoch())
                                    .count();
        key << writeTicks;
    }
    else
        key << 0;

    return (fs::path(beiklive::path::cachePath()) / "3ds_icons" /
            (Hex64(Fnv1a64(key.str())) + ".png"))
        .string();
}

std::string GetOrCreateThreeDsIconPath(const std::string& romPath)
{
    if (romPath.empty())
        return "";

    const std::string cachePath = GetThreeDsIconCachePath(romPath);
    if (cachePath.empty())
        return "";
    {
        std::error_code ec;
        if (fs::is_regular_file(cachePath, ec) && !ec)
            return cachePath;
    }

    std::vector<uint8_t> smdh;
    if (!ExtractSmdh(romPath, smdh))
        return "";

    std::vector<uint8_t> rgba;
    if (!RenderSmdhIcon(smdh, rgba))
        return "";

    std::error_code ec;
    fs::create_directories(fs::path(cachePath).parent_path(), ec);
    if (stbi_write_png(cachePath.c_str(), kIconSize, kIconSize, 4, rgba.data(),
                       kIconSize * 4) == 0)
        return "";
    return cachePath;
}

std::string ExtractThreeDsTitle(const std::string& romPath)
{
    if (romPath.empty())
        return "";
    std::vector<uint8_t> smdh;
    if (!ExtractSmdh(romPath, smdh))
        return "";
    // 简中(6) > 英文(1) > 日文(0)
    std::string title = SmdhTitle(smdh, 6);
    if (title.empty())
        title = SmdhTitle(smdh, 1);
    if (title.empty())
        title = SmdhTitle(smdh, 0);
    return title;
}

} // namespace beiklive
