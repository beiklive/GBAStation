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


    // BUTTON_B – close (pop this activity)
    registerAction("beiklive/hints/close"_i18n,
                   brls::BUTTON_B,
                   [](brls::View*) {
                       brls::Application::popActivity();
                       return true;
                   },
                   false, false, brls::SOUND_CLICK);

    // BUTTON_L – zoom out
    registerAction("beiklive/hints/zoom_out"_i18n,
                    brls::BUTTON_LB,
                    [this](brls::View*) { zoomOut(); return true; },
                    false, false, brls::SOUND_CLICK);

    // BUTTON_R – zoom in
    registerAction("beiklive/hints/zoom_in"_i18n,
                   brls::BUTTON_RB,
                   [this](brls::View*) { zoomIn(); return true; },
                   false, false, brls::SOUND_CLICK);

    // BUTTON_X – reset
    registerAction("beiklive/hints/reset"_i18n,
                   brls::BUTTON_X,
                   [this](brls::View*) { resetView(); return true; },
                   false, false, brls::SOUND_CLICK);

    // D-pad – pan
    registerAction("", brls::BUTTON_UP,
                   [this](brls::View*) { panUp();    return true; }, true);
    registerAction("", brls::BUTTON_DOWN,
                   [this](brls::View*) { panDown();  return true; }, true);
    registerAction("", brls::BUTTON_LEFT,
                   [this](brls::View*) { panLeft();  return true; }, true);
    registerAction("", brls::BUTTON_RIGHT,
                   [this](brls::View*) { panRight(); return true; }, true);

    // Swallow unused buttons so they don't propagate
    beiklive::swallow(this, brls::BUTTON_LT);
    beiklive::swallow(this, brls::BUTTON_RT);
    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_Y);
    beiklive::swallow(this, brls::BUTTON_START);
    beiklive::swallow(this, brls::BUTTON_BACK);

    // ── Start async image load ────────────────────────────────────────────────
    auto& byteCache = beiklive::UI::ImageFileCache::instance();
    if (const auto* cached = byteCache.getBytes(m_imagePath))
    {
        // Cache hit: bytes already in RAM.
        // Copy into m_asyncBytes; the NVG image is created in draw() once
        // an NVGcontext is guaranteed to be available.
        std::lock_guard<std::mutex> lock(m_asyncMutex);
        m_asyncBytes   = *cached;
        m_asyncReady.store(true);
        m_asyncLoading = false;
    }
    else
    {
        // Cache miss – read file in background, using ASYNC_RETAIN/RELEASE so
        // that the callback is safely discarded if this view is destroyed first.
        m_asyncLoading = true;
        std::string path = m_imagePath;

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path]()
        {
            // Background: read file bytes.
            std::string data;
            {
                std::ifstream f(path, std::ios::binary | std::ios::ate);
                if (f.is_open())
                {
                    auto size = static_cast<std::streamsize>(f.tellg());
                    f.seekg(0);
                    data.resize(static_cast<size_t>(size));
                    f.read(data.data(), size);
                    // Discard partial reads
                    if (f.gcount() != size)
                        data.clear();
                }
            }

            // Main thread: store bytes and signal draw() to create the texture.
            brls::sync([ASYNC_TOKEN, data = std::move(data), path]()
            {
                ASYNC_RELEASE
                this->m_asyncLoading = false;

                if (data.empty())
                {
                    this->m_loaded = true; // mark as done (failed)
                    return;
                }

                // Store in byte cache.
                std::vector<uint8_t> bytes(data.begin(), data.end());
                beiklive::UI::ImageFileCache::instance().storeBytes(path, bytes);

                // Hand off bytes to draw() for NVG image creation.
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
    // NVG images are cleaned up during NVG context teardown by borealis.
    m_nvgImage = -1;
}

void ImageView::draw(NVGcontext* vg, float x, float y, float w, float h,
                     brls::Style /*style*/, brls::FrameContext* /*ctx*/)
{
    // ── Black background ────────────────────────────────────────────────────
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFill(vg);

    // ── Consume completed async load ────────────────────────────────────────
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

    // ── Show loading placeholder ─────────────────────────────────────────────
    if (m_asyncLoading)
    {
        nvgFontSize(vg, 28.0f);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 200));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, "加载中...", nullptr);
        return;
    }

    if (!m_loaded)
        return; // draw() might be called before the async thread completes

    if (m_nvgImage < 0 || m_imgW == 0 || m_imgH == 0)
    {
        // Draw a placeholder message if the image couldn't be loaded
        nvgFontSize(vg, 24.0f);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 200));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, m_imagePath.c_str(), nullptr);
        return;
    }

    // ── Draw image centered with current scale / offset ─────────────────────
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
