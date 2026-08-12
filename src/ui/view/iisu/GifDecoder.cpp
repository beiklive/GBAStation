#include "GifDecoder.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

extern "C" {
#include <gif_lib.h>
}

namespace beiklive
{
    namespace
    {
        void clearRect(std::vector<uint8_t>& canvas, int canvasWidth,
                       int canvasHeight, const GifImageDesc& rect)
        {
            for (int y = rect.Top; y < rect.Top + rect.Height; ++y) {
                if (y < 0 || y >= canvasHeight)
                    continue;
                for (int x = rect.Left; x < rect.Left + rect.Width; ++x) {
                    if (x < 0 || x >= canvasWidth)
                        continue;
                    auto* pixel = &canvas[(static_cast<size_t>(y) * canvasWidth + x) * 4];
                    std::memset(pixel, 0, 4);
                }
            }
        }

        void composeFrame(std::vector<uint8_t>& canvas, int canvasWidth,
                          int canvasHeight, const SavedImage& image,
                          const GraphicsControlBlock& gcb,
                          ColorMapObject* globalPalette)
        {
            ColorMapObject* palette = image.ImageDesc.ColorMap
                ? image.ImageDesc.ColorMap : globalPalette;
            if (!palette || !image.RasterBits)
                return;

            const auto& rect = image.ImageDesc;
            for (int y = 0; y < rect.Height; ++y) {
                const int dstY = rect.Top + y;
                if (dstY < 0 || dstY >= canvasHeight)
                    continue;
                for (int x = 0; x < rect.Width; ++x) {
                    const int dstX = rect.Left + x;
                    if (dstX < 0 || dstX >= canvasWidth)
                        continue;
                    const int index = image.RasterBits[static_cast<size_t>(y) * rect.Width + x];
                    if (index == gcb.TransparentColor || index >= palette->ColorCount)
                        continue;
                    const GifColorType& color = palette->Colors[index];
                    auto* pixel = &canvas[(static_cast<size_t>(dstY) * canvasWidth + dstX) * 4];
                    pixel[0] = color.Red;
                    pixel[1] = color.Green;
                    pixel[2] = color.Blue;
                    pixel[3] = 255;
                }
            }
        }

        bool hasSingleLoopCount(const GifFileType* gif)
        {
            for (int imageIndex = 0; imageIndex < gif->ImageCount; ++imageIndex) {
                const SavedImage& image = gif->SavedImages[imageIndex];
                bool netscape = false;
                for (int blockIndex = 0;
                     blockIndex < image.ExtensionBlockCount; ++blockIndex) {
                    const ExtensionBlock& block = image.ExtensionBlocks[blockIndex];
                    if (block.Function == APPLICATION_EXT_FUNC_CODE &&
                        block.ByteCount == 11 && block.Bytes &&
                        std::memcmp(block.Bytes, "NETSCAPE2.0", 11) == 0) {
                        netscape = true;
                    } else if (netscape &&
                               block.Function == CONTINUE_EXT_FUNC_CODE &&
                               block.ByteCount >= 3 && block.Bytes &&
                               block.Bytes[0] == 1) {
                        const uint16_t loopCount = static_cast<uint16_t>(block.Bytes[1]) |
                            static_cast<uint16_t>(block.Bytes[2] << 8);
                        return loopCount == 1;
                    }
                }
            }
            return false;
        }

        bool decodeImpl(const std::string& path, GifDecoded& out)
        {
            int error = 0;
            GifFileType* gif = DGifOpenFileName(path.c_str(), &error);
            if (!gif)
                return false;
            const auto closeGif = [&]() { DGifCloseFile(gif, &error); };
            if (DGifSlurp(gif) == GIF_ERROR || gif->SWidth <= 0 ||
                gif->SHeight <= 0 || gif->ImageCount <= 0) {
                closeGif();
                return false;
            }

            const int width = gif->SWidth;
            const int height = gif->SHeight;
            const bool looping = !hasSingleLoopCount(gif);
            std::vector<uint8_t> canvas(static_cast<size_t>(width) * height * 4, 0);
            std::vector<uint8_t> beforePrevious;
            GraphicsControlBlock previousGcb{};
            previousGcb.TransparentColor = NO_TRANSPARENT_COLOR;
            GifImageDesc previousRect{};
            bool havePrevious = false;
            std::vector<GifFrame> frames;
            frames.reserve(static_cast<size_t>(gif->ImageCount));

            for (int i = 0; i < gif->ImageCount; ++i) {
                if (havePrevious) {
                    if (previousGcb.DisposalMode == DISPOSE_BACKGROUND) {
                        clearRect(canvas, width, height, previousRect);
                    } else if (previousGcb.DisposalMode == DISPOSE_PREVIOUS &&
                               beforePrevious.size() == canvas.size()) {
                        canvas = beforePrevious;
                    }
                }

                beforePrevious = canvas;
                SavedImage& image = gif->SavedImages[i];
                GraphicsControlBlock gcb{};
                gcb.TransparentColor = NO_TRANSPARENT_COLOR;
                if (DGifSavedExtensionToGCB(gif, i, &gcb) == GIF_ERROR)
                    gcb.TransparentColor = NO_TRANSPARENT_COLOR;
                composeFrame(canvas, width, height, image, gcb, gif->SColorMap);

                GifFrame frame;
                frame.delayMs = gcb.DelayTime > 0
                    ? static_cast<uint32_t>(gcb.DelayTime) * 10u : 100u;
                frame.rgba = canvas;
                frames.push_back(std::move(frame));
                previousGcb = gcb;
                previousRect = image.ImageDesc;
                havePrevious = true;
            }

            closeGif();
            out.width = static_cast<uint32_t>(width);
            out.height = static_cast<uint32_t>(height);
            out.looping = looping;
            out.frames = std::move(frames);
            return !out.frames.empty();
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
        GifDecoded raw;
        if (!decodeImpl(path, raw))
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
