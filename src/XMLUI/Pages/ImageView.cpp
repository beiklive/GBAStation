#include "XMLUI/Pages/ImageView.hpp"

#include "common.hpp"

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
}

void ImageView::draw(NVGcontext* vg, float x, float y, float w, float h,
                     brls::Style /*style*/, brls::FrameContext* /*ctx*/)
{
    // ── Black background ────────────────────────────────────────────────────
    nvgBeginPath(vg);
    // nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
    nvgRect(vg, x, y, w, h);
    nvgFill(vg);

    // ── Lazy-load the NVG image on first draw ───────────────────────────────
    if (!m_loaded)
    {
        m_nvgImage = nvgCreateImage(vg, m_imagePath.c_str(), 0);
        if (m_nvgImage >= 0)
            nvgImageSize(vg, m_nvgImage, &m_imgW, &m_imgH);
        m_loaded = true;
    }

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
