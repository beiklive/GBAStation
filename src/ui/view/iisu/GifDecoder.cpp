#include "GifDecoder.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace beiklive
{
    namespace
    {
        // LSB-first 位读取
        class BitReader
        {
        public:
            explicit BitReader(const std::vector<uint8_t>& data)
                : m_data(data)
            {
            }

            bool readCode(int codeSize, uint32_t& code)
            {
                if (codeSize <= 0 || codeSize > 16)
                    return false;
                if (m_bitPos + static_cast<size_t>(codeSize) >
                    m_data.size() * 8)
                    return false;
                code = 0;
                for (int i = 0; i < codeSize; ++i) {
                    const size_t bit = m_bitPos + static_cast<size_t>(i);
                    const bool on = (m_data[bit >> 3] >> (bit & 7)) & 1;
                    code |= static_cast<uint32_t>(on) << i;
                }
                m_bitPos += static_cast<size_t>(codeSize);
                return true;
            }

        private:
            const std::vector<uint8_t>& m_data;
            size_t m_bitPos = 0;
        };

        // GIF LZW 解码（含 early-change 码宽增长规则）
        bool lzwDecode(int minCodeSize, const std::vector<uint8_t>& data,
                       std::vector<uint8_t>& out, size_t expected)
        {
            if (minCodeSize < 2 || minCodeSize > 8)
                return false;

            const uint32_t clearCode = 1u << minCodeSize;
            const uint32_t endCode = clearCode + 1;
            constexpr uint32_t maxDict = 4096;

            int codeSize = minCodeSize + 1;
            uint32_t nextCode = endCode + 1;

            std::vector<uint16_t> prefix(maxDict, 0);
            std::vector<uint8_t> suffix(maxDict, 0);
            for (uint32_t i = 0; i < clearCode; ++i)
                suffix[i] = static_cast<uint8_t>(i);

            out.reserve(expected);

            auto emit = [&](uint32_t code, std::vector<uint8_t>& tmp) -> bool {
                tmp.clear();
                uint32_t c = code;
                size_t guard = 0;
                while (c >= clearCode) {
                    if (c >= nextCode || ++guard > maxDict)
                        return false;
                    tmp.push_back(suffix[c]);
                    c = prefix[c];
                }
                tmp.push_back(suffix[c]);
                std::reverse(tmp.begin(), tmp.end());
                return true;
            };

            BitReader reader(data);
            std::vector<uint8_t> tmp;
            bool havePrev = false;
            uint32_t prev = 0;

            while (true) {
                uint32_t code = 0;
                if (!reader.readCode(codeSize, code))
                    break; // 数据截断：接受已解出的部分

                if (code == clearCode) {
                    codeSize = minCodeSize + 1;
                    nextCode = endCode + 1;
                    havePrev = false;
                    continue;
                }
                if (code == endCode)
                    break;
                if (code > nextCode)
                    return false;

                std::vector<uint8_t> entry;
                if (code == nextCode) {
                    // KwKwK 情形：entry = prev 串 + prev 串首字节
                    if (!havePrev)
                        return false;
                    if (!emit(prev, tmp))
                        return false;
                    entry = tmp;
                    entry.push_back(entry[0]);
                } else {
                    if (!emit(code, entry))
                        return false;
                }

                out.insert(out.end(), entry.begin(), entry.end());

                // 建新表项：prev 串 + entry 首字节
                if (havePrev && nextCode < maxDict) {
                    prefix[nextCode] = static_cast<uint16_t>(prev);
                    suffix[nextCode] = entry[0];
                    ++nextCode;
                    // GIF 码宽提前增长规则
                    if (nextCode == (1u << codeSize) && codeSize < 12)
                        ++codeSize;
                }

                havePrev = true;
                prev = code;
            }

            if (out.size() > expected)
                out.resize(expected);
            if (out.size() < expected)
                out.resize(expected, 0);
            return true;
        }

        void composePalette(std::vector<uint8_t>& canvas,
                            int canvasWidth, int canvasHeight,
                            uint16_t left, uint16_t top,
                            uint16_t frameWidth, uint16_t frameHeight,
                            const uint8_t* indices, bool interlace,
                            const std::vector<uint8_t>& palette,
                            bool transparent, uint8_t transparentIndex)
        {
            // 行重排（交错）
            std::vector<int> rowMap(static_cast<size_t>(frameHeight));
            if (interlace) {
                int r = 0;
                for (int pass = 0; pass < 4; ++pass) {
                    const int start =
                        pass == 0 ? 0 : pass == 1 ? 4 : pass == 2 ? 2 : 1;
                    const int step = pass <= 1 ? 8 : pass == 2 ? 4 : 2;
                    for (int yy = start; yy < static_cast<int>(frameHeight);
                         yy += step)
                        rowMap[static_cast<size_t>(r++)] = yy;
                }
            } else {
                for (int yy = 0; yy < static_cast<int>(frameHeight); ++yy)
                    rowMap[static_cast<size_t>(yy)] = yy;
            }

            for (int r = 0; r < static_cast<int>(frameHeight); ++r) {
                const int dstY = static_cast<int>(top) + rowMap[static_cast<size_t>(r)];
                if (dstY < 0 || dstY >= canvasHeight)
                    continue;
                const uint8_t* row = indices +
                    static_cast<size_t>(r) * frameWidth;
                for (int xx = 0; xx < static_cast<int>(frameWidth); ++xx) {
                    const int dstX = static_cast<int>(left) + xx;
                    if (dstX < 0 || dstX >= canvasWidth)
                        continue;
                    const uint8_t idx = row[static_cast<size_t>(xx)];
                    if (transparent && idx == transparentIndex)
                        continue;
                    const uint8_t* rgb = &palette[static_cast<size_t>(idx) * 3];
                    uint8_t* p = &canvas[
                        (static_cast<size_t>(dstY) * canvasWidth +
                         static_cast<size_t>(dstX)) *
                        4];
                    p[0] = rgb[0];
                    p[1] = rgb[1];
                    p[2] = rgb[2];
                    p[3] = 255;
                }
            }
        }

        bool decodeImpl(const std::vector<uint8_t>& file, GifDecoded& out)
        {
            if (file.size() < 13)
                return false;
            if (std::memcmp(file.data(), "GIF87a", 6) != 0 &&
                std::memcmp(file.data(), "GIF89a", 6) != 0)
                return false;

            const uint16_t width = file[6] | static_cast<uint16_t>(file[7] << 8);
            const uint16_t height = file[8] | static_cast<uint16_t>(file[9] << 8);
            if (width == 0 || height == 0)
                return false;

            const uint8_t packed = file[10];
            const bool hasGct = (packed & 0x80) != 0;
            const int gctSize = 2 << (packed & 0x07);

            std::vector<uint8_t> gct;
            size_t pos = 13;
            if (hasGct) {
                const size_t bytes = static_cast<size_t>(gctSize) * 3;
                if (pos + bytes > file.size())
                    return false;
                gct.assign(file.begin() + static_cast<ptrdiff_t>(pos),
                           file.begin() + static_cast<ptrdiff_t>(pos + bytes));
                pos += bytes;
            }

            std::vector<uint8_t> canvas(static_cast<size_t>(width) * height * 4, 0);
            std::vector<uint8_t> prevCanvas;
            std::vector<GifFrame> frames;

            struct Gce
            {
                uint8_t disposal = 0;
                bool transparency = false;
                uint8_t transparentIndex = 0;
                uint16_t delayCs = 0;
            };
            Gce gce{};
            bool gcePresent = false;
            bool looping = true; // 无 NETSCAPE 时多数播放器循环播放

            while (pos < file.size()) {
                const uint8_t block = file[pos++];
                if (block == 0x3B)
                    break; // trailer
                if (block == 0x21) { // extension
                    if (pos >= file.size())
                        break;
                    const uint8_t label = file[pos++];
                    if (label == 0xF9) { // Graphic Control Extension
                        if (pos + 6 > file.size())
                            break;
                        const uint8_t len = file[pos++];
                        const uint8_t flags = file[pos++];
                        const uint16_t delay =
                            file[pos] | static_cast<uint16_t>(file[pos + 1] << 8);
                        const uint8_t tIndex = file[pos + 2];
                        pos += 3;
                        const uint8_t term = file[pos++];
                        (void)len;
                        (void)term;
                        gce.disposal = (flags >> 2) & 0x07;
                        gce.transparency = (flags & 0x01) != 0;
                        gce.transparentIndex = tIndex;
                        gce.delayCs = delay;
                        gcePresent = true;
                    } else if (label == 0xFF) { // application extension
                        // NETSCAPE2.0 循环信息
                        if (pos >= file.size())
                            break;
                        const uint8_t appLen = file[pos++];
                        std::vector<uint8_t> app(
                            file.begin() + static_cast<ptrdiff_t>(pos),
                            file.begin() + static_cast<ptrdiff_t>(pos + appLen));
                        pos += appLen;
                        bool isNetscape = app.size() == 11 &&
                            std::memcmp(app.data(), "NETSCAPE2.0", 11) == 0;
                        // 子块：首个数据块为 [0x01, loopLo, loopHi]
                        if (pos >= file.size())
                            break;
                        const uint8_t slen = file[pos++];
                        if (isNetscape && slen >= 3 &&
                            file[pos] == 0x01) {
                            const uint16_t loopCount =
                                file[pos + 1] |
                                static_cast<uint16_t>(file[pos + 2] << 8);
                            // 0 = 无限循环；1 = 只播一次
                            looping = loopCount != 1;
                        }
                        pos += slen;
                        // 跳过剩余子块
                        while (pos < file.size()) {
                            const uint8_t rest = file[pos++];
                            if (rest == 0)
                                break;
                            pos += rest;
                        }
                    } else {
                        // 注释/纯文本等：跳过子块
                        while (pos < file.size()) {
                            const uint8_t slen = file[pos++];
                            if (slen == 0)
                                break;
                            pos += slen;
                        }
                    }
                } else if (block == 0x2C) { // image descriptor
                    if (pos + 9 > file.size())
                        break;
                    const uint16_t left = file[pos] | static_cast<uint16_t>(file[pos + 1] << 8);
                    const uint16_t top = file[pos + 2] | static_cast<uint16_t>(file[pos + 3] << 8);
                    const uint16_t fw = file[pos + 4] | static_cast<uint16_t>(file[pos + 5] << 8);
                    const uint16_t fh = file[pos + 6] | static_cast<uint16_t>(file[pos + 7] << 8);
                    const uint8_t ipacked = file[pos + 8];
                    pos += 9;
                    const bool hasLct = (ipacked & 0x80) != 0;
                    const bool interlace = (ipacked & 0x40) != 0;
                    const int lctSize = 2 << (ipacked & 0x07);

                    std::vector<uint8_t> palette = gct;
                    if (hasLct) {
                        const size_t bytes = static_cast<size_t>(lctSize) * 3;
                        if (pos + bytes > file.size())
                            break;
                        palette.assign(
                            file.begin() + static_cast<ptrdiff_t>(pos),
                            file.begin() + static_cast<ptrdiff_t>(pos + bytes));
                        pos += bytes;
                    }
                    if (palette.empty() || pos >= file.size())
                        break;

                    const uint8_t minCodeSize = file[pos++];

                    // 拼接 LZW 子块
                    std::vector<uint8_t> lzw;
                    while (pos < file.size()) {
                        const uint8_t slen = file[pos++];
                        if (slen == 0)
                            break;
                        if (pos + slen > file.size())
                            break;
                        lzw.insert(lzw.end(),
                                   file.begin() + static_cast<ptrdiff_t>(pos),
                                   file.begin() + static_cast<ptrdiff_t>(pos + slen));
                        pos += slen;
                    }

                    // 上一帧 disposal 处理
                    if (gcePresent && gce.disposal == 2) {
                        std::memset(canvas.data(), 0, canvas.size());
                    } else if (gcePresent && gce.disposal == 3) {
                        if (prevCanvas.size() == canvas.size())
                            canvas = prevCanvas;
                    }
                    prevCanvas = canvas;

                    std::vector<uint8_t> indices;
                    lzwDecode(minCodeSize, lzw, indices,
                              static_cast<size_t>(fw) * fh);
                    composePalette(canvas, width, height, left, top, fw, fh,
                                   indices.data(), interlace, palette,
                                   gcePresent && gce.transparency,
                                   gce.transparentIndex);

                    GifFrame frame;
                    frame.delayMs = gcePresent
                        ? static_cast<uint32_t>(gce.delayCs) * 10
                        : 100;
                    if (frame.delayMs == 0)
                        frame.delayMs = 100;
                    frame.rgba = canvas;
                    frames.push_back(std::move(frame));

                    gcePresent = false;
                } else {
                    break; // 未知块
                }
            }

            if (frames.empty())
                return false;

            out.width = width;
            out.height = height;
            out.looping = looping;
            out.frames = std::move(frames);
            return true;
        }

        void downsampleFrame(GifFrame& frame, int srcW, int srcH,
                             int dstW, int dstH)
        {
            if (dstW == srcW && dstH == srcH)
                return;
            std::vector<uint8_t> out(static_cast<size_t>(dstW) * dstH * 4);
            for (int y = 0; y < dstH; ++y) {
                const int sy = std::min(srcH - 1, y * srcH / dstH);
                for (int x = 0; x < dstW; ++x) {
                    const int sx = std::min(srcW - 1, x * srcW / dstW);
                    const size_t s = (static_cast<size_t>(sy) * srcW + sx) * 4;
                    const size_t d = (static_cast<size_t>(y) * dstW + x) * 4;
                    out[d + 0] = frame.rgba[s + 0];
                    out[d + 1] = frame.rgba[s + 1];
                    out[d + 2] = frame.rgba[s + 2];
                    out[d + 3] = frame.rgba[s + 3];
                }
            }
            frame.rgba = std::move(out);
        }
    } // namespace

    bool GifDecoder::decode(const std::string& path, GifDecoded& out,
                            int maxEdge, size_t maxFrames)
    {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f)
            return false;
        std::fseek(f, 0, SEEK_END);
        const long size = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        if (size <= 0) {
            std::fclose(f);
            return false;
        }
        std::vector<uint8_t> file(static_cast<size_t>(size));
        if (std::fread(file.data(), 1, static_cast<size_t>(size), f) !=
            static_cast<size_t>(size)) {
            std::fclose(f);
            return false;
        }
        std::fclose(f);

        GifDecoded raw;
        if (!decodeImpl(file, raw))
            return false;

        // 帧数上限：按步长抽样
        if (maxFrames > 0 && raw.frames.size() > maxFrames) {
            const size_t stride =
                (raw.frames.size() + maxFrames - 1) / maxFrames;
            std::vector<GifFrame> sampled;
            for (size_t i = 0; i < raw.frames.size(); i += stride)
                sampled.push_back(std::move(raw.frames[i]));
            if (sampled.empty())
                sampled.push_back(std::move(raw.frames.back()));
            raw.frames = std::move(sampled);
        }

        // 等比缩放（最近邻）
        int dstW = static_cast<int>(raw.width);
        int dstH = static_cast<int>(raw.height);
        if (maxEdge > 0) {
            const int longest = std::max(dstW, dstH);
            if (longest > maxEdge) {
                const double scale =
                    static_cast<double>(maxEdge) / longest;
                dstW = std::max(1, static_cast<int>(dstW * scale));
                dstH = std::max(1, static_cast<int>(dstH * scale));
            }
        }
        for (auto& frame : raw.frames)
            downsampleFrame(frame, static_cast<int>(raw.width),
                            static_cast<int>(raw.height), dstW, dstH);

        out.width = static_cast<uint32_t>(dstW);
        out.height = static_cast<uint32_t>(dstH);
        out.looping = raw.looping;
        out.frames = std::move(raw.frames);
        return true;
    }
} // namespace beiklive
