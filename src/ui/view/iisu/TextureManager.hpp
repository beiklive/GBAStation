#pragma once

#include <borealis.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace beiklive
{
    /// 纹理资源：NanoVG 句柄 + 尺寸 + 引用计数；GIF 时持有每帧独立纹理
    struct TextureResource
    {
        std::string path;
        int imageHandle = 0; // 静态图句柄 / GIF 首帧句柄
        int width = 0;
        int height = 0;
        int refCount = 0;

        // GIF 动画（isGif=true 时有效，每帧一个独立纹理，绘制时直接切换）
        bool isGif = false;
        bool gifLooping = true;
        std::vector<int> gifFrameHandles;
        std::vector<uint32_t> gifDelaysMs;
    };

    /// 纹理缓存：首次从文件解码并创建 NanoVG 纹理，之后直接复用
    class TextureManager
    {
    public:
        /// 加载（引用计数 +1），返回 imageHandle；失败返回 0
        int loadTexture(NVGcontext* vg, const std::string& path);
        /// 释放（引用计数 -1），归零时删除纹理
        void releaseTexture(NVGcontext* vg, const std::string& path);
        /// 清空全部纹理
        void clear(NVGcontext* vg);

        // ---- GIF 动画访问 ----
        bool isGifTexture(const std::string& path) const;
        size_t gifFrameCount(const std::string& path) const;
        bool gifLooping(const std::string& path) const;
        uint32_t gifDelayMs(const std::string& path, size_t frame) const;
        /// 指定帧的独立纹理句柄（越界返回首帧句柄）
        int gifFrameTexture(const std::string& path, size_t frame) const;

    private:
        std::unordered_map<std::string, TextureResource> m_cache;
    };
} // namespace beiklive
