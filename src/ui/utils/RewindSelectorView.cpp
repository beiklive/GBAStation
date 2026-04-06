#include "RewindSelectorView.hpp"
#include "AnimationHelper.hpp"
#include "core/GameSignal.hpp"

#include <borealis.hpp>

namespace beiklive
{
    // =========================================================================
    // 辅助函数：RGB565 → RGBA8888
    // =========================================================================
    static std::vector<uint8_t> rgb565ToRgba8888(
        const std::vector<uint16_t>& src, unsigned w, unsigned h)
    {
        std::vector<uint8_t> dst(w * h * 4, 255);
        for (unsigned i = 0; i < w * h; ++i) {
            uint16_t px = src[i];
            uint8_t r = static_cast<uint8_t>(((px >> 11) & 0x1F) << 3);
            uint8_t g = static_cast<uint8_t>(((px >>  5) & 0x3F) << 2);
            uint8_t b = static_cast<uint8_t>(( px        & 0x1F) << 3);
            dst[i * 4 + 0] = r;
            dst[i * 4 + 1] = g;
            dst[i * 4 + 2] = b;
            dst[i * 4 + 3] = 255;
        }
        return dst;
    }

    // =========================================================================
    // RewindThumbItem 实现
    // =========================================================================

    RewindThumbItem::RewindThumbItem(int frameIndex, const std::vector<uint16_t>& thumb)
        : m_frameIndex(frameIndex)
    {
        setAxis(brls::Axis::COLUMN);
        setAlignItems(brls::AlignItems::CENTER);
        setJustifyContent(brls::JustifyContent::FLEX_START);
        setWidth(ITEM_W);
        setHeight(ITEM_H);
        setFocusable(true);
        setMargins(4.f, 4.f, 4.f, 4.f);
        setBorderColor(nvgRGBA(100, 100, 100, 150));
        setBorderThickness(1.f);
        setCornerRadius(4.f);
        setHideHighlightBackground(true);
        setShadowVisibility(true);
        setShadowType(brls::ShadowType::GENERIC);

        // 预先将 RGB565 转换为 RGBA8888，首次 draw 时再创建 NVG 图像
        if (!thumb.empty()) {
            m_rgbaData = rgb565ToRgba8888(
                thumb, RewindFrame::THUMB_W, RewindFrame::THUMB_H);
        }

        // 帧序号标签（底部居中）
        m_indexLabel = new brls::Label();
        char frameIndexText[16];
        std::snprintf(frameIndexText, sizeof(frameIndexText), "-%d帧", m_frameIndex);
        m_indexLabel->setText(frameIndexText);
        m_indexLabel->setFontSize(12.f);
        m_indexLabel->setTextColor(nvgRGBA(200, 200, 200, 230));
        m_indexLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_indexLabel->setFocusable(false);
        m_indexLabel->setMarginTop(4.f);
        this->addView(m_indexLabel);

        // 无缩略图占位标签（仅在没有图像数据时显示）
        if (thumb.empty()) {
            m_noThumbLabel = new brls::Label();
            m_noThumbLabel->setText("暂无画面");
            m_noThumbLabel->setFontSize(12.f);
            m_noThumbLabel->setTextColor(nvgRGBA(160, 160, 160, 200));
            m_noThumbLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            m_noThumbLabel->setFocusable(false);
            m_noThumbLabel->setGrow(1.f);
            this->addView(m_noThumbLabel);
        }

        // A 键确认：触发回调
        registerAction("确认", brls::BUTTON_A, [this](brls::View*) -> bool {
            if (onItemClicked)
                onItemClicked(m_frameIndex);
            return true;
        }, false, false, brls::SOUND_CLICK);
    }

    RewindThumbItem::~RewindThumbItem()
    {
        // NVG 图像需通过 nvgDeleteImage(vg, handle) 释放，但析构时 NVG 上下文可能已失效
        // borealis 的 NVG 后端在上下文重置时会统一回收所有已注册的图像资源
    }

    void RewindThumbItem::_createNvgImage(NVGcontext* vg)
    {
        if (m_imgCreated || m_rgbaData.empty()) {
            m_imgCreated = true;
            return;
        }
        m_nvgImage = nvgCreateImageRGBA(
            vg,
            static_cast<int>(RewindFrame::THUMB_W),
            static_cast<int>(RewindFrame::THUMB_H),
            NVG_IMAGE_NEAREST,
            m_rgbaData.data());
        m_imgCreated = true;
        // 释放本地 RGBA 副本，节省内存
        m_rgbaData.clear();
        m_rgbaData.shrink_to_fit();
    }

    void RewindThumbItem::draw(NVGcontext* vg, float x, float y, float w, float h,
                               brls::Style style, brls::FrameContext* ctx)
    {
        // 延迟创建 NVG 图像（首次 draw 时 NVG 上下文已就绪）
        if (!m_imgCreated)
            _createNvgImage(vg);

        Box::draw(vg, x, y, w, h, style, ctx);

        if (m_nvgImage != 0) {
            // 在卡片内显示缩略图（顶部留出标签高度，保持 GBA 宽高比 240:160 = 3:2）
            constexpr float labelH = 20.f;
            float imgW = w - 8.f;
            float imgH = imgW * static_cast<float>(RewindFrame::THUMB_H)
                              / static_cast<float>(RewindFrame::THUMB_W);
            float availH = h - labelH - 8.f;
            if (imgH > availH) {
                imgH = availH;
                imgW = imgH * static_cast<float>(RewindFrame::THUMB_W)
                            / static_cast<float>(RewindFrame::THUMB_H);
            }
            float imgX = x + (w - imgW) * 0.5f;
            float imgY = y + labelH + (availH - imgH) * 0.5f;

            NVGpaint paint = nvgImagePattern(vg, imgX, imgY, imgW, imgH, 0.f, m_nvgImage, 1.f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, imgX, imgY, imgW, imgH, 2.f);
            nvgFillPaint(vg, paint);
            nvgFill(vg);
        }
    }

    void RewindThumbItem::onFocusGained()
    {
        Box::onFocusGained();
        // 获得焦点时高亮边框为紫色（GBA 风格）
        setBorderColor(nvgRGBA(120, 80, 200, 255));
        setBorderThickness(2.5f);
    }

    void RewindThumbItem::onFocusLost()
    {
        Box::onFocusLost();
        setBorderColor(nvgRGBA(100, 100, 100, 150));
        setBorderThickness(1.f);
    }

    // =========================================================================
    // RewindSelectorView 实现
    // =========================================================================

    RewindSelectorView::RewindSelectorView()
    {
        _initLayout();
    }

    void RewindSelectorView::_initLayout()
    {
        setAxis(brls::Axis::COLUMN);
        setAlignItems(brls::AlignItems::CENTER);
        setJustifyContent(brls::JustifyContent::FLEX_END);
        // 不让容器自身接受焦点，由 getDefaultFocus() 将焦点导向具体卡片
        setFocusable(false);

        // ── 半透明底部面板 ──────────────────────────────────────────────────
        m_panel = new brls::Box(brls::Axis::COLUMN);
        m_panel->setWidthPercentage(100.f);
        // 高度为屏幕高度的四分之一
        m_panel->setHeightPercentage(25.f);
        m_panel->setAlignItems(brls::AlignItems::CENTER);
        m_panel->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_panel->setPadding(10.f, 16.f, 10.f, 16.f);
        m_panel->setFocusable(false);
        m_panel->setBackgroundColor(nvgRGBA(20, 20, 30, 210));
        m_panel->setCornerRadius(12.f);
        m_panel->setShadowVisibility(true);

        // ── 标题行 ──────────────────────────────────────────────────────────
        m_titleRow = new brls::Box(brls::Axis::ROW);
        m_titleRow->setWidthPercentage(100.f);
        m_titleRow->setHeight(28.f);
        m_titleRow->setAlignItems(brls::AlignItems::CENTER);
        m_titleRow->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
        m_titleRow->setFocusable(false);
        m_titleRow->setMarginBottom(6.f);

        m_titleLabel = new brls::Label();
        m_titleLabel->setText("可视化倒带");
        m_titleLabel->setFontSize(16.f);
        m_titleLabel->setTextColor(nvgRGBA(220, 220, 255, 255));

        m_hintLabel = new brls::Label();
        m_hintLabel->setText("← → 选择  A 确认  B 取消");
        m_hintLabel->setFontSize(12.f);
        m_hintLabel->setTextColor(nvgRGBA(160, 160, 180, 220));

        m_titleRow->addView(m_titleLabel);
        m_titleRow->addView(m_hintLabel);
        m_panel->addView(m_titleRow);

        // ── 横向滚动帧 ──────────────────────────────────────────────────────
        m_scrollFrame = new brls::HScrollingFrame();
        m_scrollFrame->setWidthPercentage(100.f);
        m_scrollFrame->setGrow(1.f);
        // CENTERED 模式使 getDefaultFocus() 不依赖 frame 坐标，避免布局未完成时焦点失败
        m_scrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_scrollFrame->setScrollingIndicatorVisible(false);
        m_scrollFrame->setFocusable(false);

        // 卡片容器（ROW 方向，在 HScrollingFrame 内横向排列）
        m_itemBox = new brls::Box(brls::Axis::ROW);
        m_itemBox->setAlignItems(brls::AlignItems::CENTER);
        m_itemBox->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_itemBox->setPadding(4.f, 8.f, 4.f, 8.f);
        m_itemBox->setFocusable(false);

        m_scrollFrame->setContentView(m_itemBox);
        m_panel->addView(m_scrollFrame);

        this->addView(m_panel);

        // B 键注册在父容器上，供焦点在卡片上时也能触发（通过动作冒泡）
        this->registerAction("取消", brls::BUTTON_B, [this](brls::View*) -> bool {
            if (m_onClose)
                m_onClose();
            return true;
        }, false, false, brls::SOUND_BACK);
    }

    brls::View* RewindSelectorView::getDefaultFocus()
    {
        // 焦点落在最右边的卡片（最新帧），即列表的最后一项
        if (!m_items.empty())
            return m_items.back();
        return nullptr;
    }

    void RewindSelectorView::_clearItems()
    {
        for (auto* item : m_items)
            m_itemBox->removeView(item, true);
        m_items.clear();
    }

    void RewindSelectorView::openWithFrames(
        std::vector<std::pair<int, std::vector<uint16_t>>> frames)
    {
        _clearItems();

        if (frames.empty()) {
            brls::Application::notify("暂无倒带记录");
            if (m_onClose) m_onClose();
            return;
        }

        // frames 已按时间顺序排列（最旧帧在前，最新帧在后），逐一创建缩略图卡片
        for (auto& [idx, thumb] : frames) {
            auto* item = new RewindThumbItem(idx, thumb);
            item->onItemClicked = [this](int frameIndex) {
                if (m_onFrameSelected)
                    m_onFrameSelected(frameIndex);
            };
            // 禁用 UP/DOWN 导航，保证焦点只在横向卡片列表内移动
            item->setCustomNavigationRoute(brls::FocusDirection::UP,   item);
            item->setCustomNavigationRoute(brls::FocusDirection::DOWN, item);
            m_itemBox->addView(item);
            m_items.push_back(item);
        }

        // 焦点由 getDefaultFocus() 统一管理，返回最右侧卡片（最新帧）
    }

    void RewindSelectorView::draw(NVGcontext* vg, float x, float y, float w, float h,
                                  brls::Style style, brls::FrameContext* ctx)
    {
        Box::draw(vg, x, y, w, h, style, ctx);
    }

} // namespace beiklive
