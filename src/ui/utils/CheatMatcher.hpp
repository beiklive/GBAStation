#pragma once
#include "core/common.h"
#include <vector>
#include <string>
#include <functional>

namespace beiklive {

/// 金手指自动匹配结果
struct CheatMatchResult {
    std::string filename;  ///< .cht 文件名
    std::string filePath;  ///< 本地缓存路径
    std::string content;   ///< 文件内容
};

/// 启动金手指自动匹配流程（异步）
/// @param platform  游戏平台 (EmuPlatform值)
/// @param romPath   游戏 ROM 路径
/// @param onDone    完成回调 (entry.cheatPath 可设置的新路径, 空=未选择)
void startCheatMatching(int platform, const std::string& romPath,
                        std::function<void(const std::string& cheatPath)> onDone);

} // namespace beiklive
