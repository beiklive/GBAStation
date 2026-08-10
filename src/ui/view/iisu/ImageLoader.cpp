#include "ImageLoader.hpp"

#include <borealis/extern/nanovg/stb_image.h>

namespace beiklive
{
    bool ImageLoader::load(const std::string& path,
                           std::vector<uint8_t>& data,
                           int& width, int& height)
    {
        if (path.empty())
            return false;

        int w = 0;
        int h = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
        if (!pixels || w <= 0 || h <= 0) {
            if (pixels)
                stbi_image_free(pixels);
            return false;
        }

        const size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
        data.assign(pixels, pixels + size);
        stbi_image_free(pixels);

        width = w;
        height = h;
        return true;
    }
} // namespace beiklive
