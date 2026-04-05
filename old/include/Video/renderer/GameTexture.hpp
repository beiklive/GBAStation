#pragma once

#include <glad/glad.h>

namespace beiklive {

/// 游戏帧 GL 纹理封装
///
/// 管理用于接收游戏核心 CPU 帧数据的 OpenGL 2D 纹理。
/// 支持动态调整尺寸（当游戏分辨率改变时重新分配）。
class GameTexture {
public:
    GameTexture()  = default;
    ~GameTexture() { deinit(); }

    GameTexture(const GameTexture&)            = delete;
    GameTexture& operator=(const GameTexture&) = delete;

    /// 创建或调整纹理为 @a width × @a height（RGBA8888）。
    ///
    /// 若已有纹理且尺寸一致则仅更新过滤参数；
    /// 否则销毁旧纹理并重新分配。
    ///
    /// @param width   纹理宽度（像素）。
    /// @param height  纹理高度（像素）。
    /// @param linear  true = GL_LINEAR；false = GL_NEAREST（默认）。
    /// @return true = 成功。
    bool init(unsigned width, unsigned height, bool linear = false);

    /// 释放 GL 纹理资源。
    void deinit();

    /// 设置纹理过滤模式（不改变尺寸）。
    void setFilter(bool linear);

    GLuint   texId()  const { return m_texId;  }
    unsigned width()  const { return m_width;  }
    unsigned height() const { return m_height; }
    bool     isValid() const { return m_texId != 0; }

private:
    GLuint   m_texId  = 0;
    unsigned m_width  = 0;
    unsigned m_height = 0;
};

} // namespace beiklive
