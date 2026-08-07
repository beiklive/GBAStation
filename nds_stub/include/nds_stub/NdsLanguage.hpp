#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

namespace beiklive
{
namespace nds_stub
{

/// Lightweight zh/en translation for the NDS stub menus.
/// Language comes from the launcher's UI.language (config.cfg).
class NdsLanguage
{
public:
    static NdsLanguage& Instance()
    {
        static NdsLanguage instance;
        return instance;
    }

    bool IsEnglish()
    {
        EnsureLoaded();
        return isEnglish_;
    }

    /// Translate a Chinese UI string; returns the input unchanged for zh.
    /// The returned pointer stays valid until the next call.
    const char* Tr(const char* zh)
    {
        EnsureLoaded();
        if (!isEnglish_ || !zh)
            return zh;
        auto it = table_.find(zh);
        if (it != table_.end())
            return it->second.c_str();
        return zh;
    }

private:
    NdsLanguage() = default;

    void EnsureLoaded()
    {
        if (loaded_)
            return;
        loaded_ = true;

        std::string language = "zh-CN";
        const char* paths[] = {
            "sdmc:/GBAStation/config/config.cfg",
            "/GBAStation/config/config.cfg",
        };
        for (const char* path : paths)
        {
            std::ifstream file(path);
            if (!file)
                continue;
            std::string line;
            while (std::getline(file, line))
            {
                const size_t eq = line.find('=');
                if (eq == std::string::npos)
                    continue;
                const std::string key = line.substr(0, eq);
                if (key != "UI.language")
                    continue;
                std::string value = line.substr(eq + 1);
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    value = value.substr(1, value.size() - 2);
                language = value;
                break;
            }
            break;
        }

        isEnglish_ = (language == "en-US" || language == "en" || language == "English");
        if (!isEnglish_)
            return;

        table_ = {
            {"返回游戏", "Resume Game"},
            {"保存状态", "Save State"},
            {"读取状态", "Load State"},
            {"金手指设置", "Cheats"},
            {"画面设置", "Display Settings"},
            {"重置游戏", "Reset Game"},
            {"退出游戏", "Exit Game"},
            {"游戏菜单", "Game Menu"},
            {"确定", "OK"},
            {"取消", "Cancel"},
            {"返回", "Back"},
            {"返回列表", "Back to List"},
            {"删除", "Delete"},
            {"删除即时存档", "Delete Save State"},
            {"确认删除 ss%d 及对应截图？", "Delete ss%d and its screenshot?"},
            {"设置", "Settings"},
            {"显示", "Display"},
            {"系统", "System"},
            {"保存", "Save"},
            {"读取", "Load"},
            {"进入", "Enter"},
            {"当前", "Current"},
            {"开启", "On"},
            {"关闭", "Off"},
            {"开", "On"},
            {"关", "Off"},
            {"未选择", "Not selected"},
            {"未命名目录", "Untitled Folder"},
            {"未命名金手指", "Untitled Cheat"},
            {"收起", "Collapse"},
            {"展开", "Expand"},
            {"槽位 %d", "Slot %d"},
            {"档位 %d", "Slot %d"},
            {"空存档槽", "Empty Slot"},
            {"已有状态", "Occupied"},
            {"残留截图", "Stale Screenshot"},
            {"无效状态", "Invalid State"},
            {"画面过滤", "Texture Filtering"},
            {"快进倍率", "Fast Forward Speed"},
            {"3D分辨率", "3D Resolution"},
            {"整数倍缩放", "Integer Scaling"},
            {"画面布局", "Screen Layout"},
            {"自定义画面布局", "Custom Layout"},
            {"画面方向", "Screen Orientation"},
            {"屏幕间距", "Screen Gap"},
            {"个性化设置", "Personalization"},
            {"遮罩选择", "Mask Selection"},
            {"滤镜选择", "Shader Selection"},
            {"同步设置", "Sync Settings"},
            {"同步画面设置", "Sync Display Settings"},
            {"同步遮罩设置", "Sync Mask Settings"},
            {"同步滤镜设置", "Sync Shader Settings"},
            {"同步画面设置完成", "Display Settings Synced"},
            {"同步遮罩设置完成", "Mask Settings Synced"},
            {"同步滤镜设置完成", "Shader Settings Synced"},
            {"同步当前游戏的布局、缩放到其他NDS游戏。", "Sync layout and scaling to other NDS games."},
            {"将当前游戏的遮罩数据同步到其他NDS游戏。", "Sync mask data to other NDS games."},
            {"将当前游戏的滤镜开关和滤镜名称同步到其他NDS游戏。", "Sync shader toggles and names to other NDS games."},
            {"确认后会立即开始同步。", "Syncing starts immediately after confirmation."},
            {"已同步到 %d 个游戏。", "Synced to %d games."},
            {"上屏布局", "Top Screen Layout"},
            {"下屏布局", "Bottom Screen Layout"},
            {"缩放", "Scale"},
            {"X偏移", "X Offset"},
            {"Y偏移", "Y Offset"},
            {"遮罩设置", "Mask Settings"},
            {"遮罩开关", "Mask Toggle"},
            {"遮罩路径", "Mask Path"},
            {"滤镜设置", "Shader Settings"},
            {"滤镜开关", "Shader Toggle"},
            {"滤镜类型", "Shader Type"},
            {"参数设置", "Parameter Settings"},
            {"当前滤镜暂无可调参数", "This shader has no adjustable parameters"},
            {"选择滤镜", "Select Shader"},
            {"上级目录", "Parent Directory"},
            {"文件夹", "Folder"},
            {"图片预览", "Image Preview"},
            {"预览", "Preview"},
            {"选择", "Select"},
            {"未找到当前游戏的 usrcheat.dat 金手指", "No usrcheat.dat cheats found for the current game"},
            {"纵向对称", "Vertical Symmetric"},
            {"横向对称", "Horizontal Symmetric"},
            {"上屏优先", "Top Priority"},
            {"下屏优先", "Bottom Priority"},
            {"混合横向", "Hybrid Horizontal"},
            {"单上屏", "Top Only"},
            {"单下屏", "Bottom Only"},
            {"自定义", "Custom"},
            {"0度", "0deg"},
            {"90度", "90deg"},
            {"180度", "180deg"},
            {"270度", "270deg"},
            {"默认 / 步长 %d", "Default / Step %d"},
            {"默认 %d / 步长 %d", "Default %d / Step %d"},
            {"默认 %.0f / 步长 %.0f", "Default %.0f / Step %.0f"},
            {"默认 %.*f / 步长 %.*f", "Default %.*f / Step %.*f"},
            {"B 返回   A 重置当前项", "B Back   A Reset Current"},
            {"B 返回   A 确定", "B Back   A Confirm"},
            {"B 返回   A 选择/开关   LR 调整参数", "B Back   A Select/Toggle   LR Adjust"},
            {"A 确定   B 返回", "A Confirm   B Back"},
            {"注意：1-3倍为安全分辨率，4倍可能导致卡顿、内存不足或崩溃，请酌情使用", "Note: 1-3x are safe; 4x may cause lag, OOM or crashes"},
            {"按 A 将重新加载当前游戏。", "Press A to reload the current game."},
        };
    }

    bool loaded_ = false;
    bool isEnglish_ = false;
    std::unordered_map<std::string, std::string> table_;
};

} // namespace nds_stub
} // namespace beiklive

/// Global shorthand for NDS stub translations.
#define NDS_L(zh) beiklive::nds_stub::NdsLanguage::Instance().Tr(zh)
