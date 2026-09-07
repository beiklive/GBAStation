#include "core/GameEntryDefaults.hpp"

#include "core/ThreeDsTitlePaths.hpp"
#include "core/Tools.hpp"
#include "core/common.h"
#include "core/constexpr.h"
#include "core/rom/Ps1DiscMeta.hpp"
#include "core/rom/PspMeta.hpp"
#include "core/rom/ThreeDsIcon.hpp"

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace beiklive
{

ImportDefaultsConfig buildImportDefaultsConfig(int platform)
{
    namespace sk = beiklive::SettingKey;

    ImportDefaultsConfig config;
    config.platform = platform;
    if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) ||
        platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
        return config;
    config.overlayEnabled = beiklive::tools::shouldAutoEnableOverlayForPlatform(platform);
    config.shaderEnabled = beiklive::tools::shouldAutoEnableShaderForPlatform(platform);

    std::string overlayKey = beiklive::tools::platformOverlayKey(platform);
    if (!overlayKey.empty())
        config.overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");

    std::string shaderKey = beiklive::tools::platformShaderKey(platform);
    if (!shaderKey.empty())
        config.shaderPath = GET_SETTING_KEY_STR(shaderKey.c_str(), "");
    if (config.shaderPath.empty())
        config.shaderPath = GET_SETTING_KEY_STR(sk::KEY_DISPLAY_SHADER_PATH, "");

    return config;
}

namespace
{
bool isDefaultFileNameTitle(const beiklive::GameEntry& entry, const std::string& romStem)
{
    return entry.title.empty() || entry.title == romStem;
}

std::string lookupMappingName(const std::string& stem)
{
    if (!beiklive::NameMappingManager)
        return "";
    auto value = beiklive::NameMappingManager->Get(stem);
    if (value)
    {
        if (auto str = value->AsString(); str && !str->empty())
            return *str;
    }
    return "";
}
} // namespace

void applyImportEntryDefaults(beiklive::GameEntry& entry, const ImportDefaultsConfig& cfg)
{
    const int platform = entry.platform;
    const std::string romPath = entry.path;
    if (romPath.empty())
        return;

    const fs::path romFsPath(romPath);
    const std::string romStem = romFsPath.stem().string();

    // 1. 存档目录：为空时回退 saves/<平台>/<stem> 并建目录。
    if (entry.savePath.empty())
        entry.savePath = beiklive::tools::defaultGameSavePath(platform, romPath);
    if (!entry.savePath.empty())
    {
        std::error_code ec;
        fs::create_directories(entry.savePath, ec);
    }

    // 2. 封面：仅当尚未指定（扫描/Web 空，LPL 已给定 RetroArch 缩略图则不覆盖）。
    if (entry.logoPath.empty())
    {
        std::string logoPath = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(platform), romPath);
        const char* coverExts[] = {".png", ".jpg", ".jpeg"};
        std::error_code coverEc;
        fs::path coverFile;
        for (const char* ext : coverExts)
        {
            coverEc.clear();
            coverFile = romFsPath.parent_path() / (romStem + ext);
            if (fs::exists(coverFile, coverEc) && !coverEc)
            {
                logoPath = coverFile.string();
                break;
            }
        }
        if (logoPath == beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(platform), romPath))
        {
            for (const char* ext : coverExts)
            {
                coverEc.clear();
                coverFile = romFsPath.parent_path() / "logos" / (romStem + ext);
                if (fs::exists(coverFile, coverEc) && !coverEc)
                {
                    logoPath = coverFile.string();
                    break;
                }
            }
        }
        entry.logoPath = logoPath;
    }

    // 3. 每机种元数据（标题仅在仍是默认文件名时以内嵌名覆盖；封面仅在仍为
    //    平台默认图时以内置图标覆盖；ICON0/缓存写入存档目录）。
    const bool titleStillDefault = isDefaultFileNameTitle(entry, romStem);
    const bool logoIsDefault = entry.logoPath == beiklive::tools::getDefaultLogoPath(
        static_cast<beiklive::enums::EmuPlatform>(platform), romPath);

    if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP))
    {
        if (titleStillDefault && cfg.useNameMapping)
        {
            const std::string mapped = lookupMappingName(romStem);
            if (!mapped.empty())
                entry.title = mapped;
        }
        if (titleStillDefault && entry.title == romStem)
        {
            const std::string realTitle = beiklive::psp_meta::ExtractTitle(romPath);
            if (!realTitle.empty())
                entry.title = realTitle;
        }
        if (logoIsDefault)
        {
            const std::string icon = beiklive::psp_meta::ExtractIcon0(romPath, entry.savePath);
            if (!icon.empty())
                entry.logoPath = icon;
        }
    }
    else if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
    {
        // 3DS 分支外的默认图判断内部已含 NDS 图标缓存语义。
        if (logoIsDefault)
        {
            const std::string ndsIcon = beiklive::GetOrCreateNdsIconPath(romPath);
            if (!ndsIcon.empty())
                entry.logoPath = ndsIcon;
        }
        if (titleStillDefault && entry.title == romStem)
        {
            const std::string ndsTitle = beiklive::ExtractNdsHeaderTitle(romPath);
            if (!ndsTitle.empty())
                entry.title = ndsTitle;
        }
    }
    else if (platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
    {
        if (entry.threeDsTitleId.empty())
            entry.threeDsTitleId = beiklive::three_ds::readNcsdTitleId(romPath);
        if (logoIsDefault)
        {
            const std::string icon = beiklive::GetOrCreateThreeDsIconPath(romPath);
            if (!icon.empty())
                entry.logoPath = icon;
        }
        if (titleStillDefault && entry.title == romStem)
        {
            const std::string title = beiklive::ExtractThreeDsTitle(romPath);
            if (!title.empty())
                entry.title = title;
        }
    }
    else if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPS1))
    {
        // PS1 镜像无内嵌显示名，但 SYSTEM.CNF 的 serial 稳定，可参与名称映射。
        if (cfg.resolvePs1SerialTitle && titleStillDefault && entry.title == romStem)
        {
            const std::string serial = beiklive::ps1_meta::ExtractSerial(romPath);
            if (!serial.empty())
            {
                const std::string mapped = lookupMappingName(serial);
                if (!mapped.empty())
                    entry.title = mapped;
            }
        }
    }

    // 4. 遮罩/着色器/显示默认与 NDS 屏幕默认。
    entry.overlayEnabled = cfg.overlayEnabled;
    entry.shaderEnabled = cfg.shaderEnabled;
    entry.overlayPath = cfg.overlayPath;
    entry.shaderPath = cfg.shaderPath;

    std::string mode = GET_SETTING_KEY_STR("display.mode", "original");
    if (mode == "fill")
        entry.displayMode = 1;
    else if (mode == "integer")
        entry.displayMode = 2;
    else if (mode == "custom")
        entry.displayMode = 3;
    else if (mode == "four_three" || mode == "4:3")
        entry.displayMode = 4;
    else
        entry.displayMode = 0;
    entry.integerAspectRatio =
        static_cast<float>(GET_SETTING_KEY_INT("display.integer_scale_mult", 0));

    if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
    {
        entry.ndsScreenLayout = "priority_top";
        entry.ndsScreenOrientation = "0";
        entry.ndsIntegerScale = true;
        entry.ndsScreenGap = 0;
        entry.ndsBottomOpacity = 1.0f;
    }
}

} // namespace beiklive
