#include "Video/renderer/FrameUploader.hpp"

#include <borealis.hpp>
#include <vector>

namespace beiklive {

// ============================================================
// upload – 上传 RGBA8888 数据
// ============================================================
void FrameUploader::upload(GLuint texId,
                            unsigned width, unsigned height,
                            const void* pixels,
                            unsigned texW, unsigned texH)
{
    if (!texId || !pixels || !width || !height) return;

    glBindTexture(GL_TEXTURE_2D, texId);

    if (texW == width && texH == height) {
        // 尺寸相同：仅更新像素数据
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        static_cast<GLsizei>(width),
                        static_cast<GLsizei>(height),
                        GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    } else {
        // 尺寸变化：重新分配纹理存储
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(width),
                     static_cast<GLsizei>(height),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
// uploadXRGB8888 – 上传 XRGB8888 数据（libretro 标准格式）
//
// XRGB8888 内存布局（小端序）：[B][G][R][X]（字节序从低到高）
//   即 32 位整数值 0x00RRGGBB 存储为字节序列：BB GG RR 00
//
// GL_BGRA（若可用）直接上传；否则软件转换为 RGBA。
// ============================================================
void FrameUploader::uploadXRGB8888(GLuint texId,
                                    unsigned width, unsigned height,
                                    const void* pixels, unsigned pitch,
                                    unsigned texW, unsigned texH)
{
    if (!texId || !pixels || !width || !height) return;

    const bool tightPitch = (pitch == width * 4u);

#if defined(USE_GLES2) || defined(USE_GLES3)
    // GLES 不保证支持 GL_BGRA；软件转换为 RGBA。
    // 注意：此软件转换在 CPU 上执行，已知存在性能开销。
    // 实际游戏分辨率较小（240p-480p），对帧率影响可忽略不计。
    // 如需高性能，可在 GLES3 下改用 GL_EXT_texture_format_BGRA8888 扩展。
    std::vector<uint8_t> converted(static_cast<size_t>(width) * height * 4);
    const uint8_t* src = static_cast<const uint8_t*>(pixels);
    uint8_t* dst = converted.data();
    for (unsigned row = 0; row < height; ++row) {
        const uint8_t* rowSrc = src + static_cast<size_t>(row) * pitch;
        uint8_t*       rowDst = dst + static_cast<size_t>(row) * width * 4;
        for (unsigned col = 0; col < width; ++col) {
            // 内存字节序：[B, G, R, X]  →  输出 [R, G, B, 255]
            rowDst[col * 4 + 0] = rowSrc[col * 4 + 2]; // R
            rowDst[col * 4 + 1] = rowSrc[col * 4 + 1]; // G
            rowDst[col * 4 + 2] = rowSrc[col * 4 + 0]; // B
            rowDst[col * 4 + 3] = 0xFF;                 // A
        }
    }
    glBindTexture(GL_TEXTURE_2D, texId);
    if (texW == width && texH == height) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        static_cast<GLsizei>(width),
                        static_cast<GLsizei>(height),
                        GL_RGBA, GL_UNSIGNED_BYTE, converted.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(width),
                     static_cast<GLsizei>(height),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, converted.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    // 桌面 GL：使用 GL_BGRA 直接上传（避免 CPU 软件转换）
    glBindTexture(GL_TEXTURE_2D, texId);

    if (!tightPitch) {
        // 有行间距（pitch > width*4）：设置 UNPACK_ROW_LENGTH
        glPixelStorei(GL_UNPACK_ROW_LENGTH,
                      static_cast<GLint>(pitch / 4));
    }

    if (texW == width && texH == height) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        static_cast<GLsizei>(width),
                        static_cast<GLsizei>(height),
                        GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(width),
                     static_cast<GLsizei>(height),
                     0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
    }

    if (!tightPitch) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

} // namespace beiklive
