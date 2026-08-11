#include "LayoutEditor.hpp"

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "../ImageWidget.hpp"
#include "../LayoutManager.hpp"
#include "../Widget.hpp"

namespace beiklive
{
    namespace
    {
        constexpr const char* SAVE_DIR = "GBAStation/theme/iisu";
    } // namespace

    LayoutEditor::LayoutEditor(LayoutManager* layout) : m_layout(layout)
    {
    }

    void LayoutEditor::enter()
    {
        m_active = true;
    }

    void LayoutEditor::exit()
    {
        if (m_lifted && m_layout)
            m_layout->animateItemScale(m_lifted, 1.f);
        m_lifted = nullptr;
        m_active = false;
    }

    bool LayoutEditor::toggleLift()
    {
        if (!m_active || !m_layout)
            return false;

        if (m_lifted) {
            // 放下：恢复缩放
            m_layout->animateItemScale(m_lifted, 1.f);
            m_lifted = nullptr;
            return true;
        }

        // 抬起：当前焦点 Widget 放大（浮起效果）
        LayoutItem* item = m_layout->currentItem();
        if (!item)
            return false;
        m_lifted = item;
        m_layout->animateItemScale(item, 1.15f);
        return true;
    }

    bool LayoutEditor::checkCollision(const LayoutItem& a,
                                      const LayoutItem& b) const
    {
        return a.x < b.x + b.w && a.x + a.w > b.x &&
               a.y < b.y + b.h && a.y + a.h > b.y;
    }

    bool LayoutEditor::moveItem(int dx, int dy)
    {
        if (!m_active || !m_layout)
            return false;
        // 移动当前焦点 Tile（抬起状态下即为被抬起的 Widget）
        LayoutItem* item = m_layout->currentItem();
        if (!item)
            return false;

        LayoutItem candidate = *item;
        candidate.x += dx;
        candidate.y += dy;

        const GridConfig& cfg = m_layout->grid().config();
        if (candidate.x < 0 || candidate.y < 0 ||
            candidate.x + candidate.w > cfg.columns ||
            candidate.y + candidate.h > cfg.rows)
            return false;

        for (auto& other : m_layout->items()) {
            if (&other == item || !other.visible)
                continue;
            if (checkCollision(candidate, other))
                return false;
        }

        *item = candidate;
        m_layout->focus().setCell(item->x, item->y, cfg.columns, cfg.rows);
        return true;
    }

    bool LayoutEditor::save(const std::string& fileName) const
    {
        if (!m_layout)
            return false;

        std::error_code ec;
        std::filesystem::create_directories(SAVE_DIR, ec);
        if (ec)
            return false;

        const GridConfig& cfg = m_layout->grid().config();

        nlohmann::json root;
        root["grid"] = {
            {"columns", cfg.columns},
            {"rows", cfg.rows},
        };
        nlohmann::json items = nlohmann::json::array();
        for (const auto& item : m_layout->items()) {
            if (!item.visible)
                continue;
            nlohmann::json entry;
            entry["id"] = item.id;
            entry["type"] = item.widget ? item.widget->typeName() : "empty";
            entry["data_id"] = item.widget ? item.widget->dataId() : "";
            entry["x"] = item.x;
            entry["y"] = item.y;
            entry["w"] = item.w;
            entry["h"] = item.h;
            // ImageWidget(GIF) 播放速度倍率
            if (item.widget && item.widget->typeName() == "image") {
                if (auto* image =
                        dynamic_cast<ImageWidget*>(item.widget.get())) {
                    if (image->speed() != 1.f)
                        entry["speed"] = image->speed();
                }
            }
            items.push_back(std::move(entry));
        }
        root["items"] = std::move(items);

        const std::string path =
            std::string(SAVE_DIR) + "/" + fileName;
        std::ofstream out(path);
        if (!out)
            return false;
        out << root.dump(2);
        return true;
    }

    void LayoutEditor::draw(NVGcontext* vg, int fontId)
    {
        if (!m_active || !vg || !m_layout)
            return;

        const GridConfig& cfg = m_layout->grid().config();

        // 网格辅助线（全部格子半透明显示，方便吸附）
        for (int r = 0; r < cfg.rows; ++r) {
            for (int c = 0; c < cfg.columns; ++c) {
                const GridRect cell =
                    m_layout->grid().getItemRect(c, r, 1, 1);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, cell.left, cell.top,
                               cell.width, cell.height, cfg.radius);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 10));
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 36));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
            }
        }
    }
} // namespace beiklive
