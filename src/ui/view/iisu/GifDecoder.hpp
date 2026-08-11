#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace beiklive
{
    /// GIF 单帧：画布尺寸 RGBA（width*height*4）
    struct GifFrame
    {
        std::vector<uint8_t> rgba;
        uint32_t delayMs = 0;
    };

    /// GIF 解码结果：全部帧 + 画布尺寸
    struct GifDecoded
    {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<GifFrame> frames;
        bool looping = true;
    };

    /// GIF89a 解码器（手写，无外部依赖）
    class GifDecoder
    {
    public:
        /// 解码全部帧（含缩放/帧数上限），失败返回 false
        /// maxEdge > 0 时等比缩至最长边 <= maxEdge（最近邻）
        /// maxFrames > 0 时超出按步长抽样
        static bool decode(const std::string& path, GifDecoded& out,
                           int maxEdge = 0, size_t maxFrames = 0);
    };
} // namespace beiklive
