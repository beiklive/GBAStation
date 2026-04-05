#pragma once

#include <glad/glad.h>
#include <cstdint>

namespace beiklive {

/// CPU 帧数据上传工具类
///
/// 提供将 CPU 内存中的像素数据上传至 OpenGL 纹理的静态方法。
/// 所有方法要求调用方持有有效的 GL 上下文。
class FrameUploader {
public:
    /// 将 RGBA8888 像素数据上传至已存在的 GL 纹理。
    ///
    /// 若纹理尺寸与 @a width × @a height 匹配则用 glTexSubImage2D 更新；
    /// 否则用 glTexImage2D 重新分配。
    ///
    /// @param texId   目标纹理 ID（须已绑定为 GL_TEXTURE_2D）。
    /// @param width   帧宽度（像素）。
    /// @param height  帧高度（像素）。
    /// @param pixels  RGBA8888 像素数据（每行 width × 4 字节，无填充）。
    /// @param texW    当前纹理宽度（用于判断是否需要重分配）。
    /// @param texH    当前纹理高度（用于判断是否需要重分配）。
    static void upload(GLuint texId,
                       unsigned width, unsigned height,
                       const void* pixels,
                       unsigned texW, unsigned texH);

    /// 将 XRGB8888 格式数据（libretro 标准格式）转换后上传至 GL 纹理。
    ///
    /// XRGB8888：每像素 4 字节，高字节为 X（忽略），随后为 R、G、B。
    /// 上传时转换为 GL_BGRA（若支持）或软件转换为 RGBA。
    ///
    /// @param texId   目标纹理 ID。
    /// @param width   帧宽度（像素）。
    /// @param height  帧高度（像素）。
    /// @param pixels  XRGB8888 像素数据。
    /// @param pitch   每行字节数（可大于 width × 4）。
    /// @param texW    当前纹理宽度。
    /// @param texH    当前纹理高度。
    static void uploadXRGB8888(GLuint texId,
                                unsigned width, unsigned height,
                                const void* pixels, unsigned pitch,
                                unsigned texW, unsigned texH);
};

} // namespace beiklive
