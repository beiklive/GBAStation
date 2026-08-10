#pragma once

#include <borealis.hpp>

#include <string>
#include <unordered_map>

namespace beiklive
{
    /// 纹理资源：NanoVG 句柄 + 尺寸 + 引用计数
    struct TextureResource
    {
        std::string path;
        int imageHandle = 0;
        int width = 0;
        int height = 0;
        int refCount = 0;
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

    private:
        std::unordered_map<std::string, TextureResource> m_cache;
    };
} // namespace beiklive
