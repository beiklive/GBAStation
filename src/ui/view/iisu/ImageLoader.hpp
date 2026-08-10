#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace beiklive
{
    /// 图片解码：PNG/JPG/BMP → RGBA 像素
    class ImageLoader
    {
    public:
        static bool load(const std::string& path,
                         std::vector<uint8_t>& data,
                         int& width, int& height);
    };
} // namespace beiklive
