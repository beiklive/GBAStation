#include "UI/Pages/ImageView.hpp"

#include "common.hpp"
#include "UI/Utils/ImageFileCache.hpp"

#include <borealis/core/thread.hpp>

#include <fstream>

using namespace brls::literals; // for _i18n

ImageView::ImageView(const std::string& imagePath)
    : m_imagePath(imagePath)
{
    setFocusable(true);
    setAxis(brls::Axis::COLUMN);
    setWidth(brls::View::AUTO);
    setHeight(brls::View::AUTO);
    setGrow(1.0f);
    setBackground(brls::ViewBackground::NONE);

    setHideHighlight(true);
    setHideClickAnimation(true);
    setHideHighlightBorder(true);
    setHideHighlightBackground(true);


    // B键：关闭（弹出当前活动）
    registerAction("beiklive/hints/close"_i18n,
                   brls::BUTTON_B,
                   [](brls::View*) {
                       brls::Application::popActivity();
                       return true;
                   },
                   false, false, brls::SOUND_CLICK);

    // LB键：缩小
    registerAction("beiklive/hints/zoom_out"_i18n,
                    brls::BUTTON_LB,
                    [this](brls::View*) { zoomOut(); return true; },
                    false, false, brls::SOUND_CLICK);

    // RB键：放大
    registerAction("beiklive/hints/zoom_in"_i18n,
                   brls::BUTTON_RB,
                   [this](brls::View*) { zoomIn(); return true; },
                   false, false, brls::SOUND_CLICK);

    // X键：重置
    registerAction("beiklive/hints/reset"_i18n,
                   brls::BUTTON_X,
                   [this](brls::View*) { resetView(); return true; },
                   false, false, brls::SOUND_CLICK);

    // 方向键：平移
    registerAction("", brls::BUTTON_UP,
                   [this](brls::View*) { panUp();    return true; }, true);
    registerAction("", brls::BUTTON_DOWN,
                   [this](brls::View*) { panDown();  return true; }, true);
    registerAction("", brls::BUTTON_LEFT,
                   [this](brls::View*) { panLeft();  return true; }, true);
    registerAction("", brls::BUTTON_RIGHT,
                   [this](brls::View*) { panRight(); return true; }, true);

    // 屏蔽未使用的按键，防止事件传播
    beiklive::swallow(this, brls::BUTTON_LT);
    beiklive::swallow(this, brls::BUTTON_RT);
    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_Y);
    beiklive::swallow(this, brls::BUTTON_START);
    beiklive::swallow(this, brls::BUTTON_BACK);

    // ── 开始异步图片加载 ─────────────────────────────────────────────────────
    auto& byteCache = beiklive::UI::ImageFileCache::instance();
    if (const auto* cached = byteCache.getBytes(m_imagePath))
    {
        // 缓存命中：字节已在内存中，直接使用。
        // 复制到 m_asyncBytes；NVG 图像在 draw() 首次可用时创建。
        std::lock_guard<std::mutex> lock(m_asyncMutex);
        m_asyncBytes   = *cached;
        m_asyncReady.store(true);
        m_asyncLoading = false;
    }
    else
    {
        // 缓存未命中：后台读取文件，通过 ASYNC_RETAIN/RELEASE 确保视图销毁后回调安全丢弃
        m_asyncLoading = true;
        std::string path = m_imagePath;

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path]()
        {
            // 后台线程：读取文件字节
            std::string data;
            {
                std::ifstream f(path, std::ios::binary | std::ios::ate);
                if (f.is_open())
                {
                    auto size = static_cast<std::streamsize>(f.tellg());
                    f.seekg(0);
                    data.resize(static_cast<size_t>(size));
                    f.read(data.data(), size);
                    // 丢弃不完整读取
                    if (f.gcount() != size)
                        data.clear();
                }
            }

            // 主线程：存储字节并通知 draw() 创建纹理
            brls::sync([ASYNC_TOKEN, data = std::move(data), path]()
            {
                ASYNC_RELEASE
                this->m_asyncLoading = false;

                if (data.empty())
                {
                    this->m_loaded = true; // 标记完成（加载失败）
                    return;
                }

                // 存入字节缓存
                std::vector<uint8_t> bytes(data.begin(), data.end());
                beiklive::UI::ImageFileCache::instance().storeBytes(path, bytes);

                // 将字节交给 draw() 创建 NVG 图像
                {
                    std::lock_guard<std::mutex> lock(this->m_asyncMutex);
                    this->m_asyncBytes = std::move(bytes);
                }
                this->m_asyncReady.store(true);
            });
        });
    }
}

ImageView::~ImageView()
{
    // NVG 图像在 borealis 销毁上下文时统一清理
    m_nvgImage = -1;
}

void ImageView::draw(NVGcontext* vg, float x, float y, float w, float h,
                     brls::Style /*style*/, brls::FrameContext* /*ctx*/)
{
    // ── 黑色背景 ───────────────────────────────────────────────────────────
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFill(vg);

    // ── 处理异步加载完成的结果 ──────────────────────────────────────────────
    if (m_asyncReady.load())
    {
        m_asyncReady.store(false);
        m_asyncLoading = false;

        std::vector<uint8_t> bytes;
        {
            std::lock_guard<std::mutex> lock(m_asyncMutex);
            bytes = std::move(m_asyncBytes);
        }

        if (!bytes.empty())
        {
            m_nvgImage = nvgCreateImageMem(vg, 0, bytes.data(),
                                           static_cast<int>(bytes.size()));
            if (m_nvgImage >= 0)
                nvgImageSize(vg, m_nvgImage, &m_imgW, &m_imgH);
        }
        m_loaded = true;
    }

    // ── 显示加载占位符 ──────────────────────────────────────────────────────
    if (m_asyncLoading)
    {
        nvgFontSize(vg, 28.0f);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 200));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, "加载中...", nullptr);
        return;
    }

    if (!m_loaded)
        return; // 异步线程尚未完成时 draw() 可能被提前调用

    if (m_nvgImage < 0 || m_imgW == 0 || m_imgH == 0)
    {
        // 图片加载失败时显示占位文字
        nvgFontSize(vg, 24.0f);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 200));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, m_imagePath.c_str(), nullptr);
        return;
    }

    // ── 以当前缩放/偏移居中绘制图像 ────────────────────────────────────────
    float scaledW = m_imgW * m_scale;
    float scaledH = m_imgH * m_scale;
    float imgX    = x + (w - scaledW) * 0.5f + m_offsetX;
    float imgY    = y + (h - scaledH) * 0.5f + m_offsetY;

    NVGpaint paint = nvgImagePattern(vg, imgX, imgY, scaledW, scaledH,
                                     0.0f, m_nvgImage, 1.0f);
    nvgBeginPath(vg);
    nvgRect(vg, imgX, imgY, scaledW, scaledH);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
}

void ImageView::zoomIn()
{
    m_scale = std::min(m_scale + ZOOM_STEP, ZOOM_MAX);
}

void ImageView::zoomOut()
{
    m_scale = std::max(m_scale - ZOOM_STEP, ZOOM_MIN);
}

void ImageView::resetView()
{
    m_scale   = 1.0f;
    m_offsetX = 0.0f;
    m_offsetY = 0.0f;
}

void ImageView::panLeft()  { m_offsetX -= PAN_STEP; }
void ImageView::panRight() { m_offsetX += PAN_STEP; }
void ImageView::panUp()    { m_offsetY -= PAN_STEP; }
void ImageView::panDown()  { m_offsetY += PAN_STEP; }
