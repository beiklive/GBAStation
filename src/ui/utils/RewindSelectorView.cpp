#include "RewindSelectorView.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "core/Tools.hpp"

namespace beiklive
{

// ─────────────────────────────────────────────────────────────────────────────
//  RewindThumbCard
// ─────────────────────────────────────────────────────────────────────────────

RewindThumbCard::RewindThumbCard(const std::vector<uint16_t>& thumbData,
                                  int frameIndex, const std::string& timeHint)
    : m_frameIndex(frameIndex), m_timeHint(timeHint)
{
    // 卡片固定大小（宽×高保持 3:2 比例）
    this->setWidth(120.f);
    this->setHeight(90.f);
    this->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(this);

    // 将 RGB565 转换为 RGBA8888，供 nvgCreateImageRGBA 使用
    if (!thumbData.empty() &&
        thumbData.size() >= static_cast<size_t>(RewindFrame::THUMB_W * RewindFrame::THUMB_H))
    {
        m_rgbaData.resize(RewindFrame::THUMB_W * RewindFrame::THUMB_H * 4);
        for (size_t i = 0; i < static_cast<size_t>(RewindFrame::THUMB_W * RewindFrame::THUMB_H); ++i)
        {
            uint16_t px = thumbData[i]; // RGB565
            uint8_t r = static_cast<uint8_t>((px >> 11) & 0x1F) << 3;
            uint8_t g = static_cast<uint8_t>((px >>  5) & 0x3F) << 2;
            uint8_t b = static_cast<uint8_t>( px        & 0x1F) << 3;
            m_rgbaData[i * 4 + 0] = r;
            m_rgbaData[i * 4 + 1] = g;
            m_rgbaData[i * 4 + 2] = b;
            m_rgbaData[i * 4 + 3] = 255;
        }
        m_hasThumb = true;
    }
}

RewindThumbCard::~RewindThumbCard()
{
    // NVG 图像会在 Activity 销毁时随 NVG context 一起释放，无需手动删除
}

void RewindThumbCard::onFocusGained()
{
    Box::onFocusGained();
    m_focused = true;
}

void RewindThumbCard::onFocusLost()
{
    Box::onFocusLost();
    m_focused = false;
}

void RewindThumbCard::draw(NVGcontext* vg, float x, float y, float width, float height,
                            brls::Style style, brls::FrameContext* ctx)
{
    // 外框背景
    NVGcolor bgColor = m_focused
        ? nvgRGBA(255, 255, 255, 200)
        : nvgRGBA(30, 30, 30, 180);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, 6.f);
    nvgFillColor(vg, bgColor);
    nvgFill(vg);

    // 缩略图区域（留内边距）
    constexpr float PADDING = 4.f;
    constexpr float LABEL_H = 18.f;
    float imgX = x + PADDING;
    float imgY = y + PADDING;
    float imgW = width  - PADDING * 2;
    float imgH = height - PADDING * 2 - LABEL_H;

    if (m_hasThumb && !m_rgbaData.empty()) {
        // 懒创建 NVG 图像句柄（首次 draw 时创建）
        if (m_nvgImage < 0) {
            m_nvgImage = nvgCreateImageRGBA(vg,
                static_cast<int>(RewindFrame::THUMB_W),
                static_cast<int>(RewindFrame::THUMB_H),
                0,
                m_rgbaData.data());
        }
        if (m_nvgImage >= 0) {
            NVGpaint imgPaint = nvgImagePattern(vg, imgX, imgY, imgW, imgH, 0.f, m_nvgImage, 1.f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, imgX, imgY, imgW, imgH, 4.f);
            nvgFillPaint(vg, imgPaint);
            nvgFill(vg);
        } else {
            // 创建失败：灰色占位
            nvgBeginPath(vg);
            nvgRoundedRect(vg, imgX, imgY, imgW, imgH, 4.f);
            nvgFillColor(vg, nvgRGBA(80, 80, 80, 220));
            nvgFill(vg);
        }
    } else {
        // 无缩略图：绘制灰色占位
        nvgBeginPath(vg);
        nvgRoundedRect(vg, imgX, imgY, imgW, imgH, 4.f);
        nvgFillColor(vg, nvgRGBA(60, 60, 60, 200));
        nvgFill(vg);
    }

    // 时间标签
    nvgFontSize(vg, 13.f);
    nvgFontFace(vg, "regular");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, m_focused ? nvgRGBA(30, 30, 30, 255) : nvgRGBA(220, 220, 220, 255));
    nvgText(vg, x + width * 0.5f, y + height - LABEL_H * 0.5f, m_timeHint.c_str(), nullptr);

    // 焦点高亮边框
    if (m_focused) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 1, y + 1, width - 2, height - 2, 6.f);
        nvgStrokeColor(vg, nvgRGBA(80, 160, 255, 255));
        nvgStrokeWidth(vg, 3.f);
        nvgStroke(vg);
    }

    Box::draw(vg, x, y, width, height, style, ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
//  RewindSelectorView
// ─────────────────────────────────────────────────────────────────────────────

RewindSelectorView::RewindSelectorView(GameView* gameView)
    : m_gameView(gameView)
{
    // 布局：列方向，半透明深色背景，从屏幕底部弹出
    this->setAxis(brls::Axis::COLUMN);
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setJustifyContent(brls::JustifyContent::CENTER);
    this->setFocusable(false);
    HIDE_BRLS_HIGHLIGHT(this);

    // 定位：绝对定位于屏幕底部
    this->setPositionType(brls::PositionType::ABSOLUTE);
    this->setPositionBottom(0);
    this->setPositionLeft(0);
    this->setWidthPercentage(100.f);
    this->setHeight(220.f);

    _buildHeader();
    _buildScrollArea();

    // 初始隐藏
    this->setVisibility(brls::Visibility::GONE);
}

void RewindSelectorView::draw(NVGcontext* vg, float x, float y, float width, float height,
                               brls::Style style, brls::FrameContext* ctx)
{
    // 半透明黑色背景面板
    nvgBeginPath(vg);
    nvgRect(vg, x, y, width, height);
    nvgFillColor(vg, nvgRGBA(10, 10, 10, 210));
    nvgFill(vg);

    Box::draw(vg, x, y, width, height, style, ctx);
}

void RewindSelectorView::_buildHeader()
{
    auto* titleLabel = new brls::Label();
    titleLabel->setText("◀◀ 可视化倒带   方向键选择帧   A 确认   B 返回");
    titleLabel->setFontSize(18.f);
    titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    titleLabel->setMarginBottom(8.f);
    titleLabel->setTextColor(nvgRGBA(220, 220, 220, 240));
    this->addView(titleLabel);
}

void RewindSelectorView::_buildScrollArea()
{
    m_scrollFrame = new brls::HScrollingFrame();
    m_scrollFrame->setWidth(1280.f);
    m_scrollFrame->setHeight(160.f);
    m_scrollFrame->setFocusable(false);

    m_cardBox = new brls::Box(brls::Axis::ROW);
    m_cardBox->setHeight(160.f);
    m_cardBox->setAlignItems(brls::AlignItems::CENTER);
    m_cardBox->setItemSpacing(12.f);
    m_cardBox->setPadding(10.f, 20.f, 10.f, 20.f);

    m_scrollFrame->setContentView(m_cardBox);
    this->addView(m_scrollFrame);
}

void RewindSelectorView::refreshThumbnails()
{
    if (!m_gameView || !m_cardBox) return;

    // 清除旧卡片
    m_cardBox->clearViews();

    // 从配置读取显示数量
    int thumbCount = GET_SETTING_KEY_INT("rewind.thumbnailCount", 7);
    if (thumbCount < 1) thumbCount = 1;

    // 从 GameView 均匀采样倒带帧
    auto frames = m_gameView->sampleRewindFrames(thumbCount);
    if (frames.empty()) return;

    // 估算每帧的时间偏移
    int saveInterval = GET_SETTING_KEY_INT("rewind.saveInterval", 1);
    float secondsPerFrame = saveInterval / 60.f;

    int total = static_cast<int>(frames.size());
    RewindThumbCard* firstCard = nullptr;

    for (int i = 0; i < total; ++i) {
        const RewindFrame* frame = frames[i];

        // 计算大致时间偏移（索引 0 = 最新帧，索引越大越旧）
        float secondsAgo = i * secondsPerFrame;
        std::string timeHint;
        if (secondsAgo < 1.f)
            timeHint = "当前";
        else if (secondsAgo < 60.f)
            timeHint = "-" + std::to_string(static_cast<int>(secondsAgo + 0.5f)) + "s";
        else
            timeHint = "-" + std::to_string(static_cast<int>(secondsAgo / 60.f)) + "m";

        int frameIdx = i;
        auto* card = new RewindThumbCard(frame->thumb, frameIdx, timeHint);

        // 注册 A 键：选择该帧并关闭倒带界面
        card->registerAction("确认"_i18n, brls::BUTTON_A,
            [this, frameIdx](brls::View*) {
                if (m_onSelectFrame)
                    m_onSelectFrame(frameIdx);
                // 关闭倒带并隐藏界面
                GameSignal::instance().requestRewind(false);
                GameSignal::instance().requestHideRewindUI();
                return true;
            }, false, false, brls::SOUND_CLICK);

        m_cardBox->addView(card);
        if (!firstCard) firstCard = card;
    }

    // 将焦点移到第一张卡片
    if (firstCard) {
        brls::sync([firstCard](){
            brls::Application::giveFocus(firstCard);
        });
    }
}

} // namespace beiklive

