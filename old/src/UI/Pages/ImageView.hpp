#pragma once

#include <borealis.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

/// 全屏图片查看器，支持缩放/平移。
/// 支持 PNG、JPG、JPEG、BMP、WEBP、TGA 格式。
/// - 黑色背景
/// - B 键：关闭
/// - LB：缩小；RB：放大
/// - X 键：重置缩放/平移
/// - 方向键：平移图片
/// - 其余按键全部吞噬
class ImageView : public brls::Box
{
  public:
    explicit ImageView(const std::string& imagePath);
    ~ImageView() override;

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

  private:
    std::string m_imagePath;

    // NVG 图像句柄（-1 = 未加载）
    int  m_nvgImage  = -1;
    int  m_imgW      = 0;
    int  m_imgH      = 0;
    bool m_loaded    = false;

    // 异步加载
    bool                  m_asyncLoading = false;
    std::atomic<bool>     m_asyncReady{false};
    std::vector<uint8_t>  m_asyncBytes;
    std::mutex            m_asyncMutex;

    float m_scale   = 1.0f;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;

    static constexpr float ZOOM_STEP = 0.10f;
    static constexpr float ZOOM_MIN  = 0.10f;
    static constexpr float ZOOM_MAX  = 8.0f;
    static constexpr float PAN_STEP  = 40.0f;

    void zoomIn();
    void zoomOut();
    void resetView();
    void panLeft();
    void panRight();
    void panUp();
    void panDown();
};
