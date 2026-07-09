#pragma once

#include <string>
#include <vector>

#include "mgba_stub/MgbaMenuLayer.hpp"

struct mCore;
struct mCheatDevice;

namespace beiklive::mgba_stub {

struct MgbaCheatEntry {
    std::string name;
    std::string code;
    std::string codeType;
    bool enabled = false;
    bool valid = true;
    std::string diagnostic;
};

struct MgbaCheatApplyResult {
    bool ok = true;
    int appliedCount = 0;
    int invalidCount = 0;
    std::string diagnostic;
};

std::string DefaultCheatPath(const std::string& romPath);
void AppendCheatLog(const std::string& message);
bool LoadRetroArchCheats(const std::string& path, std::vector<MgbaCheatEntry>& out);
bool SaveRetroArchCheats(const std::string& path, const std::vector<MgbaCheatEntry>& entries);
std::vector<MgbaCheatItem> BuildCheatMenuItems(const std::vector<MgbaCheatEntry>& entries);
void UpdateCheatsFromMenuItems(const std::vector<MgbaCheatItem>& items,
                               std::vector<MgbaCheatEntry>& entries);
MgbaCheatApplyResult ApplyCheatsToCore(mCore* core, std::vector<MgbaCheatEntry>& entries);
MgbaCheatApplyResult ApplyCheatsToDevice(mCheatDevice* device,
                                         int platform,
                                         std::vector<MgbaCheatEntry>& entries,
                                         const char* sourceLabel = nullptr);

} // namespace beiklive::mgba_stub
