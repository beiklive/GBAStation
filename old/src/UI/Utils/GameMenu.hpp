#pragma once

#include "common.hpp"
#include <borealis.hpp>
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/cells/cell_slider.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <functional>
#include <string>
#include <vector>

#include "Video/renderer/GLSLPParser.hpp"

// DisplayConfig.hpp 引入的类型前置声明（避免循环包含）
namespace beiklive {
    enum class ScreenMode : int;
    enum class FilterMode : int;
}

/// 金手指条目
struct CheatEntry {
    std::string desc;    ///< 金手指名称
    std::string code;    ///< 金手指代码
    bool        enabled = true; ///< 是否启用
};

class GameMenu: public brls::Box
{
public:
    /// 状态槽位信息（用于面板显示）
    struct StateSlotInfo {
        bool        exists    = false; ///< 状态文件是否存在
        std::string thumbPath;         ///< 缩略图路径（存在时非空）
    };

    GameMenu();
    ~GameMenu();

    /// 设置"返回游戏"回调。菜单关闭时在 UI 线程调用。
    void setCloseCallback(std::function<void()> cb) { m_closeCallback = std::move(cb); }

    /// 设置"退出游戏"回调。用户点击退出游戏按钮时在 UI 线程调用。
    /// 若已设置，则由回调负责实际退出（如先自动保存再 popActivity）；
    /// 若未设置，退出按钮直接调用 brls::Application::popActivity()。
    void setExitGameCallback(std::function<void()> cb) { m_exitGameCallback = std::move(cb); }

    /// 设置"重置游戏"回调。用户点击重置游戏按钮时在 UI 线程调用，触发游戏核心重置。
    void setResetGameCallback(std::function<void()> cb) { m_resetGameCallback = std::move(cb); }

    /// 设置金手指列表并刷新 cheatbox 显示。
    /// 须在主（UI）线程调用。
    void setCheats(const std::vector<CheatEntry>& cheats);

    /// 设置金手指切换回调。切换某条金手指的启用状态时调用，参数为（索引, 新状态）。
    void setCheatToggleCallback(std::function<void(int, bool)> cb)
    {
        m_cheatToggleCallback = std::move(cb);
    }

    /// 设置当前游戏文件名，用于从 gamedataManager 读取/写入游戏专属遮罩路径。
    /// 须在菜单首次显示之前调用。
    void setGameFileName(const std::string& fileName);

    /// 设置遮罩路径变更回调。用户选择新遮罩后在 UI 线程调用，参数为新的完整遮罩路径。
    void setOverlayChangedCallback(std::function<void(const std::string&)> cb)
    {
        m_overlayChangedCallback = std::move(cb);
    }

    /// 设置遮罩开关变更回调。用户切换遮罩启用状态时在 UI 线程调用，参数为新的启用状态。
    void setOverlayEnabledChangedCallback(std::function<void(bool)> cb)
    {
        m_overlayEnabledChangedCallback = std::move(cb);
    }

    /// 设置着色器开关变更回调。用户切换着色器启用状态时在 UI 线程调用，参数为新的启用状态。
    void setShaderEnabledChangedCallback(std::function<void(bool)> cb)
    {
        m_shaderEnabledChangedCallback = std::move(cb);
    }

    /// 设置着色器路径变更回调。用户选择新着色器后在 UI 线程调用，参数为新的完整路径（或空串=清空）。
    void setShaderPathChangedCallback(std::function<void(const std::string&)> cb)
    {
        m_shaderPathChangedCallback = std::move(cb);
    }

    /// 设置显示模式变更回调。用户在菜单中更改显示模式时在 UI 线程调用。
    void setDisplayModeChangedCallback(std::function<void(beiklive::ScreenMode)> cb)
    {
        m_displayModeChangedCallback = std::move(cb);
    }

    /// 设置纹理过滤变更回调。用户在菜单中更改纹理过滤时在 UI 线程调用。
    void setDisplayFilterChangedCallback(std::function<void(beiklive::FilterMode)> cb)
    {
        m_displayFilterChangedCallback = std::move(cb);
    }

    /// 设置整数缩放倍率变更回调。用户在菜单中更改整数缩放倍率时在 UI 线程调用。
    void setDisplayIntScaleChangedCallback(std::function<void(int)> cb)
    {
        m_displayIntScaleChangedCallback = std::move(cb);
    }

    /// 设置 X 坐标偏移实时变更回调。拖动滑条时持续调用，参数为像素偏移值。
    void setDisplayXOffsetChangedCallback(std::function<void(float)> cb)
    {
        m_displayXOffsetChangedCallback = std::move(cb);
    }

    /// 设置 Y 坐标偏移实时变更回调。拖动滑条时持续调用，参数为像素偏移值。
    void setDisplayYOffsetChangedCallback(std::function<void(float)> cb)
    {
        m_displayYOffsetChangedCallback = std::move(cb);
    }

    /// 设置自定义缩放实时变更回调。拖动滑条时持续调用，参数为缩放倍率。
    void setDisplayCustomScaleChangedCallback(std::function<void(float)> cb)
    {
        m_displayCustomScaleChangedCallback = std::move(cb);
    }

    /// 设置着色器参数实时变更回调。拖动参数滑条时调用，参数为（参数名, 新值）。
    void setShaderParamChangedCallback(std::function<void(const std::string&, float)> cb)
    {
        m_shaderParamChangedCallback = std::move(cb);
    }

    /// 设置保存状态回调。用户确认保存后在 UI 线程调用，参数为槽号（0-9）。
    void setSaveStateCallback(std::function<void(int)> cb) { m_saveStateCallback = std::move(cb); }

    /// 设置读取状态回调。用户确认读取后在 UI 线程调用，参数为槽号（0-9）。
    void setLoadStateCallback(std::function<void(int)> cb) { m_loadStateCallback = std::move(cb); }

    /// 设置状态槽位信息查询回调。参数为槽号（0-9），返回槽位信息（是否存在、缩略图路径）。
    void setStateInfoCallback(std::function<StateSlotInfo(int)> cb) { m_stateInfoCallback = std::move(cb); }

    /// 重置保存/读取状态面板预览图为 NoData 状态。须在 UI 线程调用。
    void refreshStatePanels();

    /// 更新着色器参数滑条区域。着色器路径变更后由 GameView 调用。
    /// 若 params 为空，则显示"无可调整参数"提示。须在 UI 线程调用。
    void updateShaderParams(const std::vector<beiklive::ShaderParamInfo>& params);

    /// 更新 X/Y 偏移和自定义缩放滑条的显示值（由 GameView 在 setGameFileName 后调用）。
    void updateDisplayOffsetSliders(float xOffset, float yOffset, float customScale);

private:
    /// 构建保存/读取状态槽位面板内容（10个槽位按钮）。
    /// 槽位按钮获得焦点时，自动更新对应预览图（m_save/loadPreviewImage）。
    void buildStatePanel(bool isSave, brls::Box* container);

    /// 根据显示模式索引更新位置/缩放设置区域和整数倍率选项的可见性。
    /// modeIdx：0=fit, 1=fill, 2=original, 3=integer, 4=custom
    void updateDisplayModeVisibility(int modeIdx);

    std::function<void()>               m_closeCallback;
    std::function<void()>               m_exitGameCallback;
    std::function<void()>               m_resetGameCallback;
    std::function<void(const std::string&)> m_overlayChangedCallback;
    std::function<void(bool)>           m_overlayEnabledChangedCallback;
    std::function<void(bool)>           m_shaderEnabledChangedCallback;
    std::function<void(const std::string&)> m_shaderPathChangedCallback;
    std::function<void(beiklive::ScreenMode)> m_displayModeChangedCallback;
    std::function<void(beiklive::FilterMode)> m_displayFilterChangedCallback;
    std::function<void(int)>            m_displayIntScaleChangedCallback;
    std::function<void(float)>          m_displayXOffsetChangedCallback;
    std::function<void(float)>          m_displayYOffsetChangedCallback;
    std::function<void(float)>          m_displayCustomScaleChangedCallback;
    std::function<void(const std::string&, float)> m_shaderParamChangedCallback;
    std::function<void(int, bool)>      m_cheatToggleCallback;
    std::function<void(int)>            m_saveStateCallback;     ///< 确认保存状态时调用
    std::function<void(int)>            m_loadStateCallback;     ///< 确认读取状态时调用
    std::function<StateSlotInfo(int)>   m_stateInfoCallback;     ///< 查询槽位信息
    std::vector<CheatEntry>             m_cheats;
    std::string                         m_romFileName;
    std::vector<beiklive::ShaderParamInfo> m_shaderParams; ///< 当前着色器参数（含实时值）
    brls::ScrollingFrame*               m_cheatScrollFrame       = nullptr;
    brls::Box*                          m_cheatItemBox           = nullptr;
    brls::ScrollingFrame*               m_displayScrollFrame     = nullptr;
    brls::DetailCell*                   m_overlayPathCell        = nullptr;
    brls::DetailCell*                   m_shaderPathCell         = nullptr; ///< 着色器路径显示单元格
    brls::Box*                          m_shaderParamBox         = nullptr; ///< 着色器参数区域容器
    brls::SelectorCell*                 m_dispModeCell           = nullptr; ///< 显示模式选择器
    brls::SelectorCell*                 m_filterCell             = nullptr; ///< 纹理过滤选择器
    brls::BooleanCell*                  m_shaderEnCell           = nullptr; ///< 着色器开关单元格
    brls::SelectorCell*                 m_intScaleCell           = nullptr; ///< 整数倍缩放选择器
    brls::SliderCell*                   m_xOffsetSlider          = nullptr; ///< X 坐标偏移滑条
    brls::SliderCell*                   m_yOffsetSlider          = nullptr; ///< Y 坐标偏移滑条
    brls::SliderCell*                   m_customScaleSlider      = nullptr; ///< 自定义缩放滑条
    brls::Box*                          m_saveStatePanel         = nullptr; ///< 保存状态面板外层容器（横向：列表+预览）
    brls::Box*                          m_loadStatePanel         = nullptr; ///< 读取状态面板外层容器（横向：列表+预览）
    brls::Box*                          m_saveStateItemBox       = nullptr; ///< 保存状态条目容器（直接用 Box，不用 ScrollingFrame）
    brls::Box*                          m_loadStateItemBox       = nullptr; ///< 读取状态条目容器（直接用 Box，不用 ScrollingFrame）
    brls::Image*                        m_savePreviewImage       = nullptr; ///< 保存状态预览图（右侧）
    brls::Image*                        m_loadPreviewImage       = nullptr; ///< 读取状态预览图（右侧）
    brls::Label*                        m_saveNoDataLabel        = nullptr; ///< 保存状态无数据提示
    brls::Label*                        m_loadNoDataLabel        = nullptr; ///< 读取状态无数据提示
    brls::Header*                       m_posScaleHeader         = nullptr; ///< 位置与缩放设置分区标题

    /// 将滑条进度值（[0,1]）映射到 X/Y 坐标偏移实际像素值（[-500, 500]）
    static constexpr float k_offsetMin  = -500.f;
    static constexpr float k_offsetMax  =  500.f;
    static constexpr float k_scaleMin   =  0.1f;
    static constexpr float k_scaleMax   =  5.0f;

    static float offsetToProgress(float v)
    {
        return (v - k_offsetMin) / (k_offsetMax - k_offsetMin);
    }
    static float progressToOffset(float p)
    {
        return k_offsetMin + p * (k_offsetMax - k_offsetMin);
    }
    static float scaleToProgress(float v)
    {
        return (v - k_scaleMin) / (k_scaleMax - k_scaleMin);
    }
    static float progressToScale(float p)
    {
        return k_scaleMin + p * (k_scaleMax - k_scaleMin);
    }
};
