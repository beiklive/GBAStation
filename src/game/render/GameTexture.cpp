#include "game/render/GameTexture.hpp"

#include <borealis.hpp>
#include <vector>

namespace beiklive {

// ============================================================
// init
// ============================================================
bool GameTexture::init(unsigned width, unsigned height, bool linear)
{
    if (width == 0 || height == 0) return false;

    // 尺寸未变时仅更新过滤参数
    if (m_texId && m_width == width && m_height == height) {
        setFilter(linear);
        return true;
    }

    // 尺寸改变：销毁旧纹理
    deinit();

    glGenTextures(1, &m_texId);
    if (!m_texId) return false;

    glBindTexture(GL_TEXTURE_2D, m_texId);

    GLenum glFilter = linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 分配 RGBA8888 空纹理（数据后续由 FrameUploader 填充）
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_width  = width;
    m_height = height;

    brls::Logger::debug("GameTexture: 分配纹理 id={} {}×{}", m_texId, width, height);
    return true;
}

// ============================================================
// deinit
// ============================================================
void GameTexture::deinit()
{
    if (m_texId) {
        glDeleteTextures(1, &m_texId);
        m_texId = 0;
    }
    m_width  = 0;
    m_height = 0;
}

// ============================================================
// setFilter
// ============================================================
void GameTexture::setFilter(bool linear)
{
    if (!m_texId) return;

    GLenum glFilter = linear ? GL_LINEAR : GL_NEAREST;
    glBindTexture(GL_TEXTURE_2D, m_texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace beiklive
