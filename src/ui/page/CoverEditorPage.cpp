#include "ui/page/CoverEditorPage.hpp"

#include "core/ThreadPool.hpp"
#include "core/common.h"
#include "core/constexpr.h"
#include "core/Translation.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/widget/Box.hpp"

#include <borealis.hpp>
#include <borealis/extern/nanovg/stb_image.h>
#include <nlohmann/json.hpp>
#include "third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

namespace beiklive
{
namespace
{
    namespace fs = std::filesystem;
    constexpr float kPi = 3.14159265358979323846f;
    constexpr int kOutputSize = 512;

    float clamp01(float value)
    {
        return std::max(0.f, std::min(1.f, value));
    }

    float smooth(float value)
    {
        value = clamp01(value);
        return value * value * (3.f - 2.f * value);
    }

    float easeOutBack(float value)
    {
        value = clamp01(value);
        constexpr float c1 = 1.35f;
        constexpr float c3 = c1 + 1.f;
        const float t = value - 1.f;
        return 1.f + c3 * t * t * t + c1 * t * t;
    }

    std::string utf8(char32_t codepoint)
    {
        std::string out;
        if (codepoint <= 0x7F) out.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return out;
    }

    unsigned char sampleChannel(const std::vector<unsigned char>& pixels,
                                int width, int height, float x, float y, int channel)
    {
        // Pixel coordinates address pixel centres. Allow the half-pixel area
        // around the outer centres and clamp it to the edge texel; otherwise a
        // cover-fitted background gets a black final row/column on export.
        if (x < -.5f || y < -.5f || x > static_cast<float>(width) - .5f ||
            y > static_cast<float>(height) - .5f)
        {
            return 0;
        }
        x = std::clamp(x, 0.f, static_cast<float>(width - 1));
        y = std::clamp(y, 0.f, static_cast<float>(height - 1));
        const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, width - 1);
        const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, height - 1);
        const int x1 = std::min(x0 + 1, width - 1);
        const int y1 = std::min(y0 + 1, height - 1);
        const float fx = x - std::floor(x);
        const float fy = y - std::floor(y);
        const auto at = [&](int sx, int sy) {
            return static_cast<float>(pixels[(static_cast<size_t>(sy) * width + sx) * 4 + channel]);
        };
        const float top = at(x0, y0) + (at(x1, y0) - at(x0, y0)) * fx;
        const float bottom = at(x0, y1) + (at(x1, y1) - at(x0, y1)) * fx;
        return static_cast<unsigned char>(std::clamp(top + (bottom - top) * fy, 0.f, 255.f));
    }

    float roundedLayerAlpha(float localX, float localY, int width, int height,
                            float normalizedScale, float cornerRadius)
    {
        if (cornerRadius <= 0.f || normalizedScale <= 0.f)
            return 1.f;
        const float radius = std::min(cornerRadius / static_cast<float>(kOutputSize) /
                                      normalizedScale,
                                      std::min(width, height) * .5f);
        const float nearestX = std::clamp(localX, -width * .5f + radius, width * .5f - radius);
        const float nearestY = std::clamp(localY, -height * .5f + radius, height * .5f - radius);
        const float dx = localX - nearestX;
        const float dy = localY - nearestY;
        return dx * dx + dy * dy <= radius * radius ? 1.f : 0.f;
    }

    struct BlurredImage
    {
        std::vector<unsigned char> pixels;
        int width = 0;
        int height = 0;
    };

    // The preview background is deliberately downscaled before the Gaussian pass:
    // it keeps memory modest for large SteamGridDB assets and makes the blur soft.
    BlurredImage makeBlurredImage(const std::vector<unsigned char>& source,
                                  int sourceWidth, int sourceHeight)
    {
        BlurredImage result;
        if (source.empty() || sourceWidth <= 0 || sourceHeight <= 0)
            return result;

        constexpr int maxDimension = 384;
        const float resize = std::min(1.f, maxDimension /
            static_cast<float>(std::max(sourceWidth, sourceHeight)));
        result.width = std::max(1, static_cast<int>(std::round(sourceWidth * resize)));
        result.height = std::max(1, static_cast<int>(std::round(sourceHeight * resize)));
        result.pixels.resize(static_cast<size_t>(result.width) * result.height * 4);
        for (int y = 0; y < result.height; ++y) {
            for (int x = 0; x < result.width; ++x) {
                const float sx = (static_cast<float>(x) + .5f) * sourceWidth / result.width - .5f;
                const float sy = (static_cast<float>(y) + .5f) * sourceHeight / result.height - .5f;
                const size_t dst = (static_cast<size_t>(y) * result.width + x) * 4;
                for (int channel = 0; channel < 4; ++channel)
                    result.pixels[dst + channel] = sampleChannel(source, sourceWidth, sourceHeight,
                                                                   sx, sy, channel);
            }
        }

        constexpr int radius = 12;
        constexpr float sigma = 5.5f;
        std::array<float, radius * 2 + 1> weights{};
        float weightTotal = 0.f;
        for (int i = -radius; i <= radius; ++i) {
            const float value = std::exp(-(i * i) / (2.f * sigma * sigma));
            weights[static_cast<size_t>(i + radius)] = value;
            weightTotal += value;
        }
        for (float& value : weights) value /= weightTotal;

        std::vector<unsigned char> horizontal(result.pixels.size());
        auto blurPass = [&](const std::vector<unsigned char>& input,
                            std::vector<unsigned char>& output, bool horizontalPass) {
            for (int y = 0; y < result.height; ++y) {
                for (int x = 0; x < result.width; ++x) {
                    const size_t dst = (static_cast<size_t>(y) * result.width + x) * 4;
                    for (int channel = 0; channel < 4; ++channel) {
                        float sum = 0.f;
                        for (int i = -radius; i <= radius; ++i) {
                            const int px = std::clamp(x + (horizontalPass ? i : 0), 0, result.width - 1);
                            const int py = std::clamp(y + (horizontalPass ? 0 : i), 0, result.height - 1);
                            sum += input[(static_cast<size_t>(py) * result.width + px) * 4 + channel] *
                                weights[static_cast<size_t>(i + radius)];
                        }
                        output[dst + channel] = static_cast<unsigned char>(std::clamp(sum, 0.f, 255.f));
                    }
                }
            }
        };
        blurPass(result.pixels, horizontal, true);
        blurPass(horizontal, result.pixels, false);
        return result;
    }

    bool writeCroppedCover(const std::vector<unsigned char>& source,
                           int sourceWidth, int sourceHeight, float scale,
                           float offsetX, float offsetY, float rotation,
                           bool includeBlurredBackground,
                           const std::vector<unsigned char>& blurredBackground,
                           int backgroundWidth, int backgroundHeight,
                           float backgroundScale, float backgroundOffsetX,
                           float backgroundOffsetY, float backgroundRotation,
                           float foregroundCornerRadius, float backgroundCornerRadius,
                           std::string& output, std::string& error)
    {
        if (source.empty() || sourceWidth <= 0 || sourceHeight <= 0 || scale <= 0.f) {
            error = "图片数据无效";
            return false;
        }

        if (includeBlurredBackground &&
            (blurredBackground.empty() || backgroundWidth <= 0 || backgroundHeight <= 0 ||
             backgroundScale <= 0.f)) {
            error = "无法生成封面背景";
            return false;
        }
        std::vector<unsigned char> target(static_cast<size_t>(kOutputSize) * kOutputSize * 4);
        const float cosR = std::cos(rotation);
        const float sinR = std::sin(rotation);
        const float outputToScreen = 1.f / static_cast<float>(kOutputSize);
        const float backgroundCos = std::cos(backgroundRotation);
        const float backgroundSin = std::sin(backgroundRotation);
        for (int y = 0; y < kOutputSize; ++y) {
            for (int x = 0; x < kOutputSize; ++x) {
                // This exactly reverses the on-screen transform used by the crop square.
                const float sx = (static_cast<float>(x) + 0.5f) * outputToScreen;
                const float sy = (static_cast<float>(y) + 0.5f) * outputToScreen;
                const float backgroundScreenX = sx - .5f - backgroundOffsetX;
                const float backgroundScreenY = sy - .5f - backgroundOffsetY;
                const float backgroundX = includeBlurredBackground
                    ? (backgroundCos * backgroundScreenX + backgroundSin * backgroundScreenY) /
                        backgroundScale + backgroundWidth * .5f : 0.f;
                const float backgroundY = includeBlurredBackground
                    ? (-backgroundSin * backgroundScreenX + backgroundCos * backgroundScreenY) /
                        backgroundScale + backgroundHeight * .5f : 0.f;
                const float screenX = sx - 0.5f - offsetX;
                const float screenY = sy - 0.5f - offsetY;
                const float localX = (cosR * screenX + sinR * screenY) / scale;
                const float localY = (-sinR * screenX + cosR * screenY) / scale;
                const float imageX = localX + sourceWidth * 0.5f;
                const float imageY = localY + sourceHeight * 0.5f;
                const size_t dst = (static_cast<size_t>(y) * kOutputSize + x) * 4;
                const float foregroundAlpha = sampleChannel(source, sourceWidth, sourceHeight,
                    imageX, imageY, 3) / 255.f * roundedLayerAlpha(
                    localX, localY, sourceWidth, sourceHeight, scale, foregroundCornerRadius);
                const float backgroundAlpha = includeBlurredBackground
                    ? sampleChannel(blurredBackground, backgroundWidth, backgroundHeight,
                                    backgroundX, backgroundY, 3) / 255.f * roundedLayerAlpha(
                        backgroundX - backgroundWidth * .5f, backgroundY - backgroundHeight * .5f,
                        backgroundWidth, backgroundHeight, backgroundScale, backgroundCornerRadius)
                    : 0.f;
                const float outputAlpha = foregroundAlpha + backgroundAlpha * (1.f - foregroundAlpha);
                for (int channel = 0; channel < 3; ++channel) {
                    const float foreground = sampleChannel(source, sourceWidth, sourceHeight,
                                                           imageX, imageY, channel);
                    const float background = includeBlurredBackground
                        ? sampleChannel(blurredBackground, backgroundWidth, backgroundHeight,
                                        backgroundX, backgroundY, channel)
                        : 0.f;
                    const float premultiplied = foreground * foregroundAlpha +
                        background * backgroundAlpha * (1.f - foregroundAlpha);
                    target[dst + channel] = outputAlpha > 0.f
                        ? static_cast<unsigned char>(std::round(premultiplied / outputAlpha)) : 0;
                }
                target[dst + 3] = static_cast<unsigned char>(std::round(outputAlpha * 255.f));
            }
        }

        const fs::path dir = fs::path(beiklive::path::cachePath()) / "cutlog";
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            error = ec.message();
            return false;
        }
        const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        output = (dir / ("cover_" + std::to_string(stamp) + ".png")).string();
        if (stbi_write_png(output.c_str(), kOutputSize, kOutputSize, 4,
                           target.data(), kOutputSize * 4) == 0) {
            error = "裁剪图片写入失败";
            return false;
        }
        return true;
    }

    class CoverEditorPage final : public beiklive::Box
    {
    public:
        CoverEditorPage(GameEntry entry, std::string sourcePath,
                        std::function<void(const std::string&)> onCoverChanged)
            : Box(brls::Axis::COLUMN)
            , m_entry(std::move(entry))
            , m_sourcePath(std::move(sourcePath))
            , m_onCoverChanged(std::move(onCoverChanged))
        {
            showHeader(false);
            showFooter(false);
            setGrow(1.f);
            setFocusable(true);
            setBackground(brls::ViewBackground::NONE);
            HIDE_BRLS_HIGHLIGHT(this);
            setCustomNavigationRoute(brls::FocusDirection::UP, this);
            setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
            setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
            setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

            int channels = 0;
            unsigned char* decoded = stbi_load(m_sourcePath.c_str(), &m_imageWidth,
                                               &m_imageHeight, &channels, 4);
            if (decoded && m_imageWidth > 0 && m_imageHeight > 0) {
                const size_t bytes = static_cast<size_t>(m_imageWidth) * m_imageHeight * 4;
                m_pixels.assign(decoded, decoded + bytes);
                stbi_image_free(decoded);
                auto blurred = makeBlurredImage(m_pixels, m_imageWidth, m_imageHeight);
                m_blurPixels = std::move(blurred.pixels);
                m_blurWidth = blurred.width;
                m_blurHeight = blurred.height;
            } else {
                if (decoded) stbi_image_free(decoded);
                m_loadError = L("无法读取该图片");
            }

            auto move = [this](float dx, float dy) {
                if (!m_saving && m_loadError.empty()) {
                    if (m_editingBackground) {
                        m_backgroundOffsetX += dx;
                        m_backgroundOffsetY += dy;
                    } else {
                        m_offsetX += dx;
                        m_offsetY += dy;
                    }
                    m_press = 1.f;
                }
                return true;
            };
            registerAction("", brls::BUTTON_LEFT, [move](brls::View*) { return move(-18.f, 0.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_LEFT, [move](brls::View*) { return move(-18.f, 0.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_RIGHT, [move](brls::View*) { return move(18.f, 0.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_RIGHT, [move](brls::View*) { return move(18.f, 0.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_UP, [move](brls::View*) { return move(0.f, -18.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_UP, [move](brls::View*) { return move(0.f, -18.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_DOWN, [move](brls::View*) { return move(0.f, 18.f); },
                           true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_DOWN, [move](brls::View*) { return move(0.f, 18.f); },
                           true, true, brls::SOUND_NONE);
            registerAction(L("缩小"), brls::BUTTON_LB, [this](brls::View*) {
                if (!m_saving) {
                    float& zoom = m_editingBackground ? m_backgroundZoom : m_zoom;
                    zoom = std::max(0.25f, zoom / 1.08f);
                }
                return true;
            }, true, true, brls::SOUND_NONE);
            registerAction(L("放大"), brls::BUTTON_RB, [this](brls::View*) {
                if (!m_saving) {
                    float& zoom = m_editingBackground ? m_backgroundZoom : m_zoom;
                    zoom = std::min(8.f, zoom * 1.08f);
                }
                return true;
            }, true, true, brls::SOUND_NONE);
            registerAction(L("左旋转"), brls::BUTTON_LT, [this](brls::View*) {
                if (!m_saving) {
                    float& rotation = m_editingBackground ? m_backgroundRotation : m_rotation;
                    rotation -= 3.f * kPi / 180.f;
                }
                return true;
            }, true, true, brls::SOUND_NONE);
            registerAction(L("右旋转"), brls::BUTTON_RT, [this](brls::View*) {
                if (!m_saving) {
                    float& rotation = m_editingBackground ? m_backgroundRotation : m_rotation;
                    rotation += 3.f * kPi / 180.f;
                }
                return true;
            }, true, true, brls::SOUND_NONE);
            registerAction(L("高斯底图"), brls::BUTTON_Y, [this](brls::View*) {
                if (!m_saving && m_loadError.empty()) {
                    m_showBlurredBackground = !m_showBlurredBackground;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
                }
                return true;
            }, false, false, brls::SOUND_NONE);
            registerAction(L("更换底图"), brls::BUTTON_X, [this](brls::View*) {
                if (!m_saving && m_loadError.empty()) _chooseBackgroundImage();
                return true;
            }, false, false, brls::SOUND_NONE);
            registerAction(L("减小圆角"), brls::BUTTON_LSB, [this](brls::View*) {
                if (!m_saving) {
                    float& radius = m_editingBackground
                        ? m_backgroundCornerRadius : m_foregroundCornerRadius;
                    radius = std::max(0.f, radius - 8.f);
                }
                return true;
            }, false, false, brls::SOUND_NONE);
            registerAction(L("增大圆角"), brls::BUTTON_RSB, [this](brls::View*) {
                if (!m_saving) {
                    float& radius = m_editingBackground
                        ? m_backgroundCornerRadius : m_foregroundCornerRadius;
                    radius = std::min(224.f, radius + 8.f);
                }
                return true;
            }, false, false, brls::SOUND_NONE);
            registerAction(L("切换编辑图层"), brls::BUTTON_START, [this](brls::View*) {
                if (!m_saving && m_loadError.empty()) {
                    m_editingBackground = !m_editingBackground;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
                }
                return true;
            }, false, false, brls::SOUND_NONE);
            registerAction(L("裁剪并保存"), brls::BUTTON_A, [this](brls::View*) {
                if (!m_saving && m_loadError.empty()) _confirmSave();
                return true;
            }, false, false, brls::SOUND_NONE);
            registerAction(L("取消编辑"), brls::BUTTON_B, [this](brls::View*) {
                if (!m_saving) _confirmCancel();
                return true;
            }, false, false, brls::SOUND_NONE);
            m_lastFrame = std::chrono::steady_clock::now();
        }

        ~CoverEditorPage() override
        {
            m_alive->store(false);
            if (m_imageHandle > 0) {
                if (auto* vg = brls::Application::getNVGContext())
                    nvgDeleteImage(vg, m_imageHandle);
            }
            if (m_blurHandle > 0) {
                if (auto* vg = brls::Application::getNVGContext())
                    nvgDeleteImage(vg, m_blurHandle);
            }
        }

        void frame(brls::FrameContext* ctx) override
        {
            Box::frame(ctx);
            const auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;
            if (dt <= 0.f || dt > .25f) dt = .016f;
            m_entrance = std::min(1.f, m_entrance + dt * 4.8f);
            m_press = std::max(0.f, m_press - dt * 5.f);
            m_time += dt;
            invalidate();
        }

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override
        {
            Box::draw(vg, x, y, w, h, style, ctx);
            if (!vg) return;
            _ensureResources(vg);
            const float alpha = smooth(m_entrance);
            const float eased = easeOutBack(m_entrance);
            nvgSave(vg);
            nvgGlobalAlpha(vg, alpha);

            const NVGpaint background = nvgLinearGradient(vg, x, y, x + w, y + h,
                nvgRGBA(11, 17, 29, 255), nvgRGBA(20, 31, 48, 255));
            nvgBeginPath(vg); nvgRect(vg, x, y, w, h); nvgFillPaint(vg, background); nvgFill(vg);
            _drawDecor(vg, x, y, w, h);

            const float square = std::clamp(std::min(w * .47f, h - 172.f), 260.f, 490.f);
            m_lastCropSize = square;
            const float cropX = x + w * .54f - square * .5f;
            const float cropY = y + h * .5f - square * .5f + 12.f;
            const float centerX = cropX + square * .5f;
            const float centerY = cropY + square * .5f;
            if (!m_transformReady && m_imageWidth > 0 && m_imageHeight > 0) {
                m_baseScale = std::max(square / m_imageWidth, square / m_imageHeight);
                m_transformReady = true;
            }
            if (!m_backgroundTransformReady && m_blurWidth > 0 && m_blurHeight > 0) {
                m_backgroundBaseScale = std::max(square / m_blurWidth, square / m_blurHeight);
                m_backgroundTransformReady = true;
            }

            _drawTop(vg, x, y, w, alpha);
            _drawEditor(vg, x, y, w, h, cropX, cropY, square, centerX, centerY, eased);
            _drawControlPanel(vg, x, y, w, h, cropX, cropY, square, alpha);
            _drawHints(vg, x, y, w, h, alpha);
            if (m_saving) _drawSaving(vg, x, y, w, h);
            nvgRestore(vg);
        }

    private:
        GameEntry m_entry;
        std::string m_sourcePath;
        std::function<void(const std::string&)> m_onCoverChanged;
        std::vector<unsigned char> m_pixels;
        std::vector<unsigned char> m_blurPixels;
        int m_imageWidth = 0;
        int m_imageHeight = 0;
        int m_blurWidth = 0;
        int m_blurHeight = 0;
        int m_imageHandle = -1;
        int m_blurHandle = -1;
        int m_defaultFont = -1;
        int m_materialFont = -1;
        int m_switchFont = -1;
        float m_entrance = 0.f;
        float m_time = 0.f;
        float m_press = 0.f;
        float m_baseScale = 1.f;
        float m_zoom = 1.f;
        float m_offsetX = 0.f;
        float m_offsetY = 0.f;
        float m_rotation = 0.f;
        float m_backgroundBaseScale = 1.f;
        float m_backgroundZoom = 1.f;
        float m_backgroundOffsetX = 0.f;
        float m_backgroundOffsetY = 0.f;
        float m_backgroundRotation = 0.f;
        bool m_transformReady = false;
        bool m_backgroundTransformReady = false;
        bool m_saving = false;
        bool m_showBlurredBackground = true;
        bool m_editingBackground = false;
        float m_foregroundCornerRadius = 0.f;
        float m_backgroundCornerRadius = 0.f;
        std::string m_loadError;
        std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);
        std::chrono::steady_clock::time_point m_lastFrame;

        void _ensureResources(NVGcontext* vg)
        {
            if (m_defaultFont < 0) m_defaultFont = brls::Application::getDefaultFont();
            if (m_materialFont < 0) m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
            if (m_switchFont < 0) m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
            if (m_imageHandle < 0 && !m_pixels.empty())
                m_imageHandle = nvgCreateImageRGBA(vg, m_imageWidth, m_imageHeight, 0, m_pixels.data());
            if (m_blurHandle < 0 && !m_blurPixels.empty())
                m_blurHandle = nvgCreateImageRGBA(vg, m_blurWidth, m_blurHeight, 0, m_blurPixels.data());
        }

        void _chooseBackgroundImage()
        {
            const fs::path currentImage(m_sourcePath);
            beiklive::openFilePicker({"png", "jpg", "jpeg"},
                [this](const std::string& selectedPath) {
                    _setBackgroundImage(selectedPath);
                },
                currentImage.parent_path().string(), currentImage.filename().string());
        }

        void _setBackgroundImage(const std::string& sourcePath)
        {
            int width = 0;
            int height = 0;
            int channels = 0;
            unsigned char* decoded = stbi_load(sourcePath.c_str(), &width, &height, &channels, 4);
            if (!decoded || width <= 0 || height <= 0) {
                if (decoded) stbi_image_free(decoded);
                brls::Application::notify(L("无法读取底图图片"));
                return;
            }
            const size_t bytes = static_cast<size_t>(width) * height * 4;
            std::vector<unsigned char> source(decoded, decoded + bytes);
            stbi_image_free(decoded);
            auto blurred = makeBlurredImage(source, width, height);
            if (blurred.pixels.empty()) {
                brls::Application::notify(L("无法生成高斯底图"));
                return;
            }
            if (m_blurHandle > 0) {
                if (auto* vg = brls::Application::getNVGContext())
                    nvgDeleteImage(vg, m_blurHandle);
                m_blurHandle = -1;
            }
            m_blurPixels = std::move(blurred.pixels);
            m_blurWidth = blurred.width;
            m_blurHeight = blurred.height;
            m_backgroundZoom = 1.f;
            m_backgroundOffsetX = 0.f;
            m_backgroundOffsetY = 0.f;
            m_backgroundRotation = 0.f;
            m_backgroundCornerRadius = 0.f;
            m_backgroundTransformReady = false;
            m_showBlurredBackground = true;
            m_editingBackground = true;
            brls::Application::notify(L("已更换高斯底图"));
        }

        void _drawDecor(NVGcontext* vg, float x, float y, float w, float h)
        {
            nvgBeginPath(vg); nvgCircle(vg, x + w * .92f, y + 55.f, 220.f);
            nvgFillColor(vg, nvgRGBA(48, 163, 255, 14)); nvgFill(vg);
            nvgBeginPath(vg); nvgCircle(vg, x + 75.f, y + h - 38.f, 165.f);
            nvgFillColor(vg, nvgRGBA(116, 87, 255, 13)); nvgFill(vg);
        }

        void _drawTop(NVGcontext* vg, float x, float y, float w, float alpha)
        {
            nvgFontFaceId(vg, m_materialFont); nvgFontSize(vg, 33.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(96, 195, 255, 245));
            const std::string icon = utf8(beiklive::material::IMAGE);
            nvgText(vg, x + 40.f, y + 44.f, icon.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 27.f);
            nvgFillColor(vg, nvgRGBA(246, 249, 255, 250));
            nvgText(vg, x + 85.f, y + 43.f,
                    (m_editingBackground ? L("编辑高斯底图") : L("编辑游戏封面")).c_str(), nullptr);
            nvgFontSize(vg, 15.f); nvgFillColor(vg, nvgRGBA(186, 204, 222, 220));
            nvgText(vg, x + 86.f, y + 68.f,
                    (m_editingBackground
                        ? L("正在调整柔化底图；+ 键可切回前景封面")
                        : L("正在调整前景封面；+ 键可切换到底图")).c_str(), nullptr);
            nvgBeginPath(vg); nvgMoveTo(vg, x + 32.f, y + 87.f); nvgLineTo(vg, x + w - 32.f, y + 87.f);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, static_cast<unsigned char>(45.f * alpha)));
            nvgStrokeWidth(vg, 1.f); nvgStroke(vg);
        }

        void _drawEditor(NVGcontext* vg, float x, float y, float w, float h,
                         float cropX, float cropY, float square, float centerX,
                         float centerY, float eased)
        {
            const float panelX = cropX - 30.f;
            const float panelY = cropY - 30.f;
            const float panelSize = square + 60.f;
            const NVGpaint shadow = nvgBoxGradient(vg, panelX + 4.f, panelY + 7.f,
                panelSize, panelSize, 18.f, 8.f, nvgRGBA(0, 0, 0, 105), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg); nvgRect(vg, panelX - 5.f, panelY - 5.f, panelSize + 16.f, panelSize + 18.f);
            nvgRoundedRect(vg, panelX, panelY, panelSize, panelSize, 18.f); nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow); nvgFill(vg);
            nvgBeginPath(vg); nvgRoundedRect(vg, panelX, panelY, panelSize, panelSize, 18.f);
            nvgFillColor(vg, nvgRGBA(7, 11, 19, 240)); nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(135, 211, 255, 105)); nvgStrokeWidth(vg, 1.f); nvgStroke(vg);

            nvgSave(vg);
            nvgIntersectScissor(vg, cropX, cropY, square, square);
            if (m_imageHandle > 0 && m_transformReady) {
                // A matching blurred image always covers the square below the
                // editable image. It is only revealed when the foreground is
                // moved, rotated, or zoomed below the crop bounds.
                if (m_showBlurredBackground && m_blurHandle > 0 && m_backgroundTransformReady) {
                    const float fillScale = m_backgroundBaseScale * m_backgroundZoom;
                    nvgSave(vg);
                    nvgTranslate(vg, centerX + m_backgroundOffsetX,
                                 centerY + m_backgroundOffsetY);
                    nvgRotate(vg, m_backgroundRotation);
                    nvgScale(vg, fillScale, fillScale);
                    const float backgroundRadius = std::min(
                        std::min(m_blurWidth, m_blurHeight) * .5f,
                        m_backgroundCornerRadius / kOutputSize * square / fillScale);
                    const NVGpaint blurred = nvgImagePattern(vg, -m_blurWidth * .5f, -m_blurHeight * .5f,
                        static_cast<float>(m_blurWidth), static_cast<float>(m_blurHeight), 0.f, m_blurHandle, 1.f);
                    nvgBeginPath(vg);
                    if (backgroundRadius > 0.f)
                        nvgRoundedRect(vg, -m_blurWidth * .5f, -m_blurHeight * .5f,
                                       static_cast<float>(m_blurWidth), static_cast<float>(m_blurHeight), backgroundRadius);
                    else
                        nvgRect(vg, -m_blurWidth * .5f, -m_blurHeight * .5f,
                                static_cast<float>(m_blurWidth), static_cast<float>(m_blurHeight));
                    nvgFillPaint(vg, blurred); nvgFill(vg);
                    nvgRestore(vg);
                }
                const float scale = m_baseScale * m_zoom;
                nvgTranslate(vg, centerX + m_offsetX, centerY + m_offsetY);
                nvgRotate(vg, m_rotation);
                nvgScale(vg, scale, scale);
                const float foregroundRadius = std::min(
                    std::min(m_imageWidth, m_imageHeight) * .5f,
                    m_foregroundCornerRadius / kOutputSize * square / scale);
                const NVGpaint image = nvgImagePattern(vg, -m_imageWidth * .5f, -m_imageHeight * .5f,
                    static_cast<float>(m_imageWidth), static_cast<float>(m_imageHeight), 0.f, m_imageHandle, 1.f);
                nvgBeginPath(vg);
                if (foregroundRadius > 0.f)
                    nvgRoundedRect(vg, -m_imageWidth * .5f, -m_imageHeight * .5f,
                                   static_cast<float>(m_imageWidth), static_cast<float>(m_imageHeight), foregroundRadius);
                else
                    nvgRect(vg, -m_imageWidth * .5f, -m_imageHeight * .5f,
                            static_cast<float>(m_imageWidth), static_cast<float>(m_imageHeight));
                nvgFillPaint(vg, image); nvgFill(vg);
            } else {
                nvgBeginPath(vg); nvgRect(vg, cropX, cropY, square, square);
                nvgFillColor(vg, nvgRGBA(26, 36, 52, 255)); nvgFill(vg);
                if (!m_loadError.empty()) {
                    nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 18.f);
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(vg, nvgRGBA(244, 194, 194, 245));
                    nvgText(vg, centerX, centerY, m_loadError.c_str(), nullptr);
                }
            }
            nvgRestore(vg);

            for (int i = 1; i < 3; ++i) {
                const float p = square * i / 3.f;
                nvgBeginPath(vg); nvgMoveTo(vg, cropX + p, cropY); nvgLineTo(vg, cropX + p, cropY + square);
                nvgMoveTo(vg, cropX, cropY + p); nvgLineTo(vg, cropX + square, cropY + p);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 54)); nvgStrokeWidth(vg, 1.f); nvgStroke(vg);
            }
            nvgBeginPath(vg); nvgRect(vg, cropX, cropY, square, square);
            nvgStrokeColor(vg, nvgRGBA(145, 220, 255, 245)); nvgStrokeWidth(vg, 2.f); nvgStroke(vg);
            const float corner = 25.f;
            nvgStrokeWidth(vg, 4.f); nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 245));
            for (const auto& p : std::array<std::pair<float, float>, 4>{
                     std::make_pair(cropX, cropY), std::make_pair(cropX + square, cropY),
                     std::make_pair(cropX, cropY + square), std::make_pair(cropX + square, cropY + square)}) {
                const float sx = p.first == cropX ? 1.f : -1.f;
                const float sy = p.second == cropY ? 1.f : -1.f;
                nvgBeginPath(vg); nvgMoveTo(vg, p.first + sx * corner, p.second); nvgLineTo(vg, p.first, p.second);
                nvgLineTo(vg, p.first, p.second + sy * corner); nvgStroke(vg);
            }
            nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 14.f); nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(212, 228, 243, 215));
            nvgText(vg, centerX, cropY + square + 48.f, L("导出 512 × 512 PNG").c_str(), nullptr);
        }

        void _drawControlPanel(NVGcontext* vg, float x, float y, float w, float h,
                               float cropX, float cropY, float square, float alpha)
        {
            const float panelW = std::max(220.f, cropX - x - 90.f);
            const float panelX = x + 40.f;
            const float panelY = y + 125.f;
            const float panelH = std::min(390.f, h - 190.f);
            const NVGpaint panelFill = nvgLinearGradient(vg, panelX, panelY,
                panelX, panelY + panelH, nvgRGBA(29, 42, 61, 245), nvgRGBA(14, 22, 35, 245));
            nvgBeginPath(vg); nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 16.f);
            nvgFillPaint(vg, panelFill); nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(158, 218, 255, 74)); nvgStrokeWidth(vg, 1.f); nvgStroke(vg);
            nvgBeginPath(vg); nvgRoundedRect(vg, panelX + 1.f, panelY + 1.f, 5.f, panelH - 2.f, 3.f);
            nvgFillColor(vg, nvgRGBA(82, 190, 255, 230)); nvgFill(vg);

            nvgFontFaceId(vg, m_materialFont); nvgFontSize(vg, 26.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(111, 207, 255, 250));
            const std::string settings = utf8(beiklive::material::SETTINGS);
            nvgText(vg, panelX + 23.f, panelY + 35.f, settings.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 20.f);
            nvgFillColor(vg, nvgRGBA(247, 250, 255, 248));
            nvgText(vg, panelX + 57.f, panelY + 34.f, L("编辑工具").c_str(), nullptr);
            nvgFontSize(vg, 13.f); nvgFillColor(vg, nvgRGBA(173, 201, 225, 225));
            nvgText(vg, panelX + 24.f, panelY + 59.f,
                    (m_editingBackground ? L("当前操作将作用于高斯底图") : L("当前操作将作用于前景封面")).c_str(), nullptr);
            nvgBeginPath(vg); nvgRoundedRect(vg, panelX + panelW - 88.f, panelY + 20.f, 67.f, 25.f, 12.f);
            nvgFillColor(vg, nvgRGBA(91, 192, 255, 36)); nvgFill(vg);
            nvgFontSize(vg, 11.f); nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, m_showBlurredBackground
                ? nvgRGBA(151, 222, 255, 245) : nvgRGBA(188, 197, 211, 225));
            nvgText(vg, panelX + panelW - 54.f, panelY + 33.f,
                    (m_showBlurredBackground ? L("底图开") : L("底图关")).c_str(), nullptr);

            struct ControlRow { const char* key; std::string title; std::string detail; NVGcolor color; };
            const std::array<ControlRow, 5> rows = {{
                {"+", L("切换编辑图层"), L("在前景封面与高斯底图间切换"), nvgRGBA(244, 142, 195, 245)},
                {"◎", L("构图定位"), L("方向键 / 摇杆移动当前图层"), nvgRGBA(91, 193, 255, 245)},
                {"L/R  ZL/ZR", L("缩放与旋转"), L("调整当前图层的大小与角度"), nvgRGBA(149, 130, 255, 245)},
                {"Y", L("高斯底图"), L("开启或关闭背景显示与导出"), nvgRGBA(80, 205, 182, 245)},
                {"X", L("更换底图"), L("从当前图片所在目录选择图片"), nvgRGBA(255, 171, 98, 245)},
            }};
            float rowY = panelY + 90.f;
            const float rowH = 46.f;
            for (const auto& row : rows) {
                nvgBeginPath(vg); nvgRoundedRect(vg, panelX + 17.f, rowY, panelW - 34.f, rowH - 5.f, 10.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 12)); nvgFill(vg);
                nvgBeginPath(vg); nvgRoundedRect(vg, panelX + 25.f, rowY + 8.f, 62.f, rowH - 15.f, 8.f);
                nvgFillColor(vg, nvgRGBA(row.color.r * 255, row.color.g * 255, row.color.b * 255, 38)); nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(row.color.r * 255, row.color.g * 255, row.color.b * 255, 120));
                nvgStrokeWidth(vg, 1.f); nvgStroke(vg);
                nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 13.f); nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, row.color); nvgText(vg, panelX + 56.f, rowY + 23.f, row.key, nullptr);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE); nvgFontSize(vg, 15.f);
                nvgFillColor(vg, nvgRGBA(240, 246, 255, 244)); nvgText(vg, panelX + 103.f, rowY + 15.f, row.title.c_str(), nullptr);
                nvgFontSize(vg, 12.f); nvgFillColor(vg, nvgRGBA(166, 191, 214, 230));
                nvgText(vg, panelX + 103.f, rowY + 31.f, row.detail.c_str(), nullptr);
                rowY += rowH;
            }
            const float activeZoom = m_editingBackground ? m_backgroundZoom : m_zoom;
            const float degrees = (m_editingBackground ? m_backgroundRotation : m_rotation) * 180.f / kPi;
            const float activeCornerRadius = m_editingBackground
                ? m_backgroundCornerRadius : m_foregroundCornerRadius;
            nvgBeginPath(vg); nvgRoundedRect(vg, panelX + 17.f, panelY + panelH - 44.f, panelW - 34.f, 27.f, 8.f);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 26)); nvgFill(vg);
            nvgFontSize(vg, 12.f); nvgFillColor(vg, nvgRGBA(158, 192, 220, 230));
            const std::string stat = (m_editingBackground ? L("底图") : L("前景")) + L("  缩放 ") +
                std::to_string(static_cast<int>(activeZoom * 100.f)) + "%   " +
                L("旋转 ") + std::to_string(static_cast<int>(degrees)) + "°   " +
                L("圆角 ") + std::to_string(static_cast<int>(activeCornerRadius)) +
                L("  (LSB/RSB)");
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(vg, panelX + panelW * .5f, panelY + panelH - 30.f, stat.c_str(), nullptr);
        }

        void _drawHint(NVGcontext* vg, brls::ControllerButton button, const char* label,
                       float& cursor, float y, float alpha)
        {
            nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 15.f);
            float bounds[4]{}; nvgTextBounds(vg, 0, 0, label, nullptr, bounds);
            cursor -= bounds[2] - bounds[0] + 40.f;
            nvgFontFaceId(vg, m_switchFont); nvgFontSize(vg, 25.f); nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, static_cast<unsigned char>(242.f * alpha)));
            const std::string glyph = brls::Hint::getKeyIcon(button); nvgText(vg, cursor + 11.f, y, glyph.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 15.f); nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(vg, cursor + 27.f, y, label, nullptr); cursor -= 11.f;
        }

        void _drawHints(NVGcontext* vg, float x, float y, float w, float h, float alpha)
        {
            float cursor = x + w - 28.f;
            const float hintY = y + h - 29.f;
            _drawHint(vg, brls::BUTTON_B, L("取消").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_Y,
                      (m_showBlurredBackground ? L("关闭底图") : L("开启底图")).c_str(),
                      cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_X, L("更换底图").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_START,
                      (m_editingBackground ? L("编辑前景") : L("编辑底图")).c_str(),
                      cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_A, L("裁剪并保存").c_str(), cursor, hintY, alpha);
        }

        void _drawSaving(NVGcontext* vg, float x, float y, float w, float h)
        {
            nvgBeginPath(vg); nvgRect(vg, x, y, w, h); nvgFillColor(vg, nvgRGBA(0, 0, 0, 155)); nvgFill(vg);
            nvgBeginPath(vg); nvgRoundedRect(vg, x + w * .5f - 190.f, y + h * .5f - 48.f, 380.f, 96.f, 16.f);
            nvgFillColor(vg, nvgRGBA(28, 40, 58, 252)); nvgFill(vg);
            nvgFontFaceId(vg, m_defaultFont); nvgFontSize(vg, 19.f); nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(241, 246, 255, 245)); nvgText(vg, x + w * .5f, y + h * .5f, L("正在裁剪并保存封面...").c_str(), nullptr);
        }

        void _confirmSave()
        {
            auto* dialog = new brls::Dialog(L("将中央方形区域设为游戏封面？"));
            dialog->addButton(L("取消"), [this]() { brls::Application::giveFocus(this); });
            dialog->addButton(L("确认裁剪"), [this]() { _save(); });
            dialog->open();
        }

        void _confirmCancel()
        {
            auto* dialog = new brls::Dialog(L("退出编辑并取消本次封面设置？"));
            dialog->addButton(L("继续编辑"), [this]() { brls::Application::giveFocus(this); });
            dialog->addButton(L("退出编辑"), [this]() {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
            });
            dialog->open();
        }

        void _save()
        {
            if (m_saving || !m_transformReady) return;
            m_saving = true;
            const float squareScale = std::max(m_lastCropSize, 1.f);
            const float normalizedScale = (m_baseScale * m_zoom) / squareScale;
            const float normalizedOffsetX = m_offsetX / squareScale;
            const float normalizedOffsetY = m_offsetY / squareScale;
            auto alive = m_alive;
            auto pixels = m_pixels;
            const int width = m_imageWidth;
            const int height = m_imageHeight;
            const float rotation = m_rotation;
            const bool includeBlurredBackground = m_showBlurredBackground;
            auto blurredBackground = m_blurPixels;
            const int backgroundWidth = m_blurWidth;
            const int backgroundHeight = m_blurHeight;
            const float backgroundScale = (m_backgroundBaseScale * m_backgroundZoom) / squareScale;
            const float backgroundOffsetX = m_backgroundOffsetX / squareScale;
            const float backgroundOffsetY = m_backgroundOffsetY / squareScale;
            const float backgroundRotation = m_backgroundRotation;
            const float foregroundCornerRadius = m_foregroundCornerRadius;
            const float backgroundCornerRadius = m_backgroundCornerRadius;
            const GameEntry entry = m_entry;
            ThreadPool::instance().enqueuePriority([this, alive, pixels = std::move(pixels), width, height,
                                                    normalizedScale, normalizedOffsetX, normalizedOffsetY,
                                                    rotation, includeBlurredBackground,
                                                    blurredBackground = std::move(blurredBackground),
                                                    backgroundWidth, backgroundHeight, backgroundScale,
                                                    backgroundOffsetX, backgroundOffsetY, backgroundRotation,
                                                    foregroundCornerRadius, backgroundCornerRadius, entry]() mutable {
                std::string output;
                std::string error;
                const bool ok = writeCroppedCover(pixels, width, height, normalizedScale,
                                                  normalizedOffsetX, normalizedOffsetY, rotation,
                                                  includeBlurredBackground,
                                                  blurredBackground, backgroundWidth, backgroundHeight,
                                                  backgroundScale, backgroundOffsetX, backgroundOffsetY,
                                                  backgroundRotation, foregroundCornerRadius,
                                                  backgroundCornerRadius,
                                                  output, error);
                brls::sync([this, alive, ok, output = std::move(output), error = std::move(error), entry]() {
                    if (!alive->load()) return;
                    m_saving = false;
                    if (!ok) {
                        brls::Application::notify(error.empty() ? L("封面保存失败") : error);
                        return;
                    }
                    if (beiklive::GameDB) {
                        beiklive::GameDB->set(entry.path, "logoPath", nlohmann::json(output));
                        beiklive::GameDB->flush();
                    }
                    if (m_onCoverChanged) m_onCoverChanged(output);
                    brls::Application::notify(L("封面已裁剪并保存"));
                    brls::Application::popActivity(brls::TransitionAnimation::NONE);
                });
            });
        }

        float m_lastCropSize = 1.f;
    };
}

void openCoverEditorPage(const GameEntry& entry, const std::string& sourcePath,
                         std::function<void(const std::string&)> onCoverChanged)
{
    auto* page = new CoverEditorPage(entry, sourcePath, std::move(onCoverChanged));
    auto* frame = new brls::AppletFrame(page);
    HIDE_BRLS_BAR(frame);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame), brls::TransitionAnimation::NONE);
    brls::Application::giveFocus(page);
}
}
