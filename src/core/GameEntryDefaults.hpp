#pragma once

#include <string>

namespace beiklive
{
struct GameEntry;

// 入库默认装配配置：扫描导入 / LPL 导入 / Web 上传三条入库通道共用，
// 保证同一机种入库字段行为一致。
struct ImportDefaultsConfig
{
    int platform = -1;
    bool overlayEnabled = false; // 设置页默认遮罩自动开启
    bool shaderEnabled = false;  // 设置页默认着色器自动开启
    std::string overlayPath;     // 平台默认遮罩路径（空 key 平台为空）
    std::string shaderPath;      // 平台默认着色器路径（空 key 平台回退全局）
    bool useNameMapping = false;         // PSP：名称映射优先于内嵌标题
    bool resolvePs1SerialTitle = false;  // PS1 标题仍为默认文件名时按 serial 查映射
};

// 按设置页装配默认遮罩/着色器开关与路径（NDS/3DS 不自动套用）。
// 语义与 DataManagementPage 原 buildSharedConfig 一致。
ImportDefaultsConfig buildImportDefaultsConfig(int platform);

// 对已设置 path/title/platform 的条目执行统一入库装配：
// 1. savePath 为空时使用 saves/<平台>/<stem> 并建目录；
// 2. logoPath 为空时按 ROM 同目录同名图 → logos/ 子目录 → 平台默认图 的顺序补封面；
// 3. PSP(ICON0+PARAM.SFO)/NDS(内置图标+ROM头标题)/3DS(SMDH 图标+标题) 元数据，
//    仅当标题仍为默认文件名(或空)时以内嵌标题覆盖，仅当封面为平台默认图时以内置图标覆盖；
//    3DS 缺 titleId 时读取 NCSD TitleID；
//    4. 遮罩/着色器/显示默认(display.mode/整数倍)与 NDS 屏幕默认写入。
void applyImportEntryDefaults(beiklive::GameEntry& entry, const ImportDefaultsConfig& cfg);

} // namespace beiklive
