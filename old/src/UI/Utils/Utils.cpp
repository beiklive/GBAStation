#include "UI/Utils/Utils.hpp"
#include "common.hpp"
#include <cmath>

namespace beiklive::UI
{
        
    BBox::BBox()
    {
        beiklive::InsertBackground(this);
    }

    BBox::BBox(brls::Axis flexDirection)
    {
        beiklive::InsertBackground(this);
        
    }




    BrowserHeader::BrowserHeader()
    {
        
        this->setJustifyContent(brls::JustifyContent::CENTER);
    
        this->setHeight(GET_STYLE("beiklive/header/header_height"));
        this->setPaddingTop(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
        this->setPaddingBottom(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
        this->setMarginRight(GET_STYLE("brls/applet_frame/padding_sides"));
        this->setMarginLeft(GET_STYLE("brls/applet_frame/padding_sides"));
    
        this->setPaddingRight(GET_STYLE("brls/applet_frame/header_padding_sides"));
        this->setPaddingLeft(GET_STYLE("brls/applet_frame/header_padding_sides"));
    
        this->setLineColor(GET_THEME_COLOR("brls/applet_frame/separator"));
        this->setLineBottom(1.f);
        
    
        m_titleBox = new brls::Box();
        m_titleBox->setMarginRight(GET_STYLE("brls/applet_frame/header_image_title_spacing"));
    
        m_titleLabel = new brls::Label();
        float FontSize = GET_STYLE("brls/applet_frame/header_title_font_size");
        m_titleLabel->setFontSize(FontSize + 15);
        
        m_titleBox->addView(m_titleLabel);
        this->addView(m_titleBox);
    
        m_subtitleBox = new brls::Box(brls::Axis::COLUMN);
        m_subtitleBox->setJustifyContent(brls::JustifyContent::CENTER);
    
        m_pathLabel = new brls::Label();
        m_pathLabel->setFontSize(FontSize/2 + 2);
        m_pathLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    
        m_subtitleBox->addView(new brls::Padding());
        m_subtitleBox->addView(m_pathLabel);
        this->addView(m_subtitleBox);
        
        this->addView(new brls::Padding());
    
        m_infoLabel = new brls::Label();
        m_infoLabel->setFontSize(FontSize);
        this->addView(m_infoLabel);
    
    }
    
    void BrowserHeader::setTitle(const std::string& title)
    {
        if (m_titleLabel)
            m_titleLabel->setText(title);
    }
    void BrowserHeader::setPath(const std::string& path)
    {
        if (m_pathLabel)
            m_pathLabel->setText(path);
    }
    void BrowserHeader::setInfo(const std::string& info)
    {
        if (m_infoLabel)
            m_infoLabel->setText(info);
    }



    // ── RoundButton 常量 ─────────────────────────────────────────────────────
    static constexpr float ROUND_BTN_IMAGE_SIZE = 80.0f;  // 圆形区域直径
    static constexpr float ROUND_BTN_PADDING    = 10.0f;  // 控件整体内边距
    static constexpr float ROUND_BTN_GAP        =  6.0f;  // 圆形与标签间距
    static constexpr float ROUND_BTN_LABEL_FS   = 20.0f;  // 标签字号
    static constexpr float ROUND_BTN_LABEL_H    = 20.0f;  // 标签行保留高度

    // ── RoundButton ──────────────────────────────────────────────────────────

    RoundButton::RoundButton(const std::string& imagePath,
                             const std::string& text,
                             std::function<void()> onClick)
        : brls::Box(brls::Axis::COLUMN)
        , m_onClick(std::move(onClick))
    {

        this->setAlignItems(brls::AlignItems::CENTER);
        this->setPadding(ROUND_BTN_PADDING);
        this->setHideHighlightBackground(true);
        this->setHideClickAnimation(true);
        this->setWidth(ROUND_BTN_IMAGE_SIZE + ROUND_BTN_PADDING * 2);
        this->setHeight(ROUND_BTN_PADDING * 2
                        + ROUND_BTN_IMAGE_SIZE
                        + ROUND_BTN_GAP
                        + ROUND_BTN_LABEL_H);

        // ── 圆形图片容器 ──
        // setCornerRadius(radius) 使其呈圆形并裁剪子视图
        m_imageWrapper = new brls::Box();
        
        m_imageWrapper->setWidth(ROUND_BTN_IMAGE_SIZE);
        m_imageWrapper->setHeight(ROUND_BTN_IMAGE_SIZE);
        m_imageWrapper->setFocusable(true);
        m_imageWrapper->setHideHighlightBackground(true);
        m_imageWrapper->setShadowVisibility(true);
        m_imageWrapper->setShadowType(brls::ShadowType::GENERIC);

        m_imageWrapper->setHighlightCornerRadius(ROUND_BTN_IMAGE_SIZE/ 2.0f);
        m_imageWrapper->setCornerRadius(ROUND_BTN_IMAGE_SIZE / 2.0f);

        m_imageWrapper->setAlignItems(brls::AlignItems::CENTER);
        m_imageWrapper->setJustifyContent(brls::JustifyContent::CENTER);
        m_imageWrapper->setBackgroundColor(nvgRGBA(40, 40, 40, 10));



        m_image = new brls::Image();
        m_image->setImageFromFile(imagePath);
        m_image->setWidth(ROUND_BTN_IMAGE_SIZE-35);
        m_image->setHeight(ROUND_BTN_IMAGE_SIZE-35);
        m_image->setScalingType(brls::ImageScalingType::FIT);
        m_image->setInterpolation(brls::ImageInterpolation::NEAREST);

        m_imageWrapper->addView(m_image);
        this->addView(m_imageWrapper);

        
        m_label = new brls::Label();
        m_label->setText(text);
        m_label->setMarginTop(20.f);
        m_label->setFontSize(ROUND_BTN_LABEL_FS);
        m_label->setTextColor(GET_THEME_COLOR("brls/text"));
        m_label->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_label->setVisibility(brls::Visibility::INVISIBLE); // INVISIBLE 保留布局空间
        this->addView(m_label);

        
        this->registerAction("OK", brls::BUTTON_A, [this](brls::View*) -> bool {
            if (!m_clickAnimating) {
                m_clickAnimating = true;
                m_clickT         = 0.0f;
                m_clickScale     = 1.0f;
                this->invalidate();
            }
            return true;
        });
        addGestureRecognizer(new brls::TapGestureRecognizer(this));
    }

    void RoundButton::onChildFocusGained(brls::View* directChild, brls::View* focusedView)
    {
        Box::onChildFocusGained(directChild, focusedView);
        m_focused = true;
        m_label->setVisibility(brls::Visibility::VISIBLE);
        this->invalidate();
    }

    void RoundButton::onChildFocusLost(brls::View* directChild, brls::View* focusedView)
    {
        Box::onChildFocusLost(directChild, focusedView);
        m_focused = false;
        m_label->setVisibility(brls::Visibility::INVISIBLE);
        this->invalidate();
    }

    // brls::View* RoundButton::getDefaultFocus()
    // {
    //     // Keep focus on the RoundButton itself rather than delegating to children
    //     return this;
    // }

    void RoundButton::setImage(const std::string& imagePath)
    {
        if (m_image)
            m_image->setImageFromFile(imagePath);
    }

    void RoundButton::setText(const std::string& text)
    {
        if (m_label)
            m_label->setText(text);
    }

    void RoundButton::draw(NVGcontext* vg, float x, float y, float w, float h,
                           brls::Style style, brls::FrameContext* ctx)
    {
        
        // ── 1. 焦点缩放动画（向1.0聚焦/0.9未聚焦插值） ──
        float target = m_focused ? 1.0f : 0.9f;
        float delta  = target - m_scale;
        if (std::abs(delta) > 0.002f) {
            m_scale += delta * 0.25f;
            this->invalidate();
        } else {
            m_scale = target;
        }

        // ── 2. 点击弹性动画 ──
        if (m_clickAnimating) {
            m_clickT += 1.0f / 60.0f;

            if (m_clickT < 0.06f) {
                // 压缩阶段：缩放至 0.88
                m_clickScale = 1.0f - 0.12f * (m_clickT / 0.06f);
            } else {
                // 阻尼弹簧回弹：A * e^(-bt) * sin(wt)
                float u      = m_clickT - 0.06f;
                m_clickScale = 1.0f + 0.14f * std::exp(-14.0f * u) * std::sin(45.0f * u);

                if (u > 0.30f && std::abs(m_clickScale - 1.0f) < 0.003f) {
                    m_clickScale     = 1.0f;
                    m_clickAnimating = false;
                    if (m_onClick)
                        m_onClick(); // 动画完成后触发回调
                }
            }
            this->invalidate();
        }

        // ── 3. 以图片圆心为基准应用合并缩放变换 ──
        float finalScale = m_scale * m_clickScale;
        float cx         = x + w * 0.5f;
        float cy         = y + ROUND_BTN_PADDING + ROUND_BTN_IMAGE_SIZE * 0.5f;
        float radius     = ROUND_BTN_IMAGE_SIZE * 0.5f;

        nvgSave(vg);
        nvgTranslate(vg,  cx,  cy);
        nvgScale(vg, finalScale, finalScale);
        nvgTranslate(vg, -cx, -cy);

        // ── 4. 绘制 borealis 子节点（m_imageWrapper + m_label） ──
        //       m_imageWrapper 通过圆角半径裁剪渲染 brls::Image
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        // ── 5. 圆形描边：聚焦时青色高亮，否则细灰边 ──
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, radius);
        nvgStrokeWidth(vg, m_focused ? 2.5f : 1.5f);
        nvgStrokeColor(vg, m_focused
            ? nvgRGBA(79, 193, 255, 230)
            : nvgRGBA(120, 120, 120, 100));
        nvgStroke(vg);

        nvgRestore(vg);
    }

    // ── ButtonBar ────────────────────────────────────────────────────────────    
    ButtonBar::ButtonBar() : brls::Box(brls::Axis::ROW)
    {
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);

    }

    void ButtonBar::addButton(const std::string& imagePath,
                              const std::string& text,
                              std::function<void()> onClick)
    {
        auto* btn = new RoundButton(imagePath, text, std::move(onClick));
        if (!this->getChildren().empty())
            btn->setMarginLeft(8.0f);
        this->addView(btn);
    }

    void ButtonBar::addButton(RoundButton* button)
    {
        if (!this->getChildren().empty())
            button->setMarginLeft(8.0f);
        this->addView(button);
    }

} // namespace beiklive::UI

