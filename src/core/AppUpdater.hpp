#pragma once

#include <string>
#include <functional>
#include <atomic>

namespace beiklive {

struct UpdateInfo {
    std::string version;     // 最新版本号（如 "v0.0.5_2"）
    std::string changelog;   // 更新日志
    std::string downloadUrl; // 下载链接
    size_t      fileSize = 0;// 文件大小（字节）
    bool hasUpdate = false;  // 是否有更新
};

class AppUpdater {
public:
    static AppUpdater& instance();

    /// 异步检查更新，不阻塞
    void check(const std::string& localVersion);

    /// 同步检查更新，返回是否有更新
    bool checkSync(const std::string& localVersion);

    /// 获取最新版本信息
    const UpdateInfo& info() const { return m_info; }

    /// 是否有可用更新
    bool hasUpdate() const { return m_info.hasUpdate; }

    /// 下载 NRO 到临时目录（Switch 平台），返回是否成功
    /// onProgress: (totalBytes, downloadedBytes) → 返回 false 可中断
    bool download(std::function<bool(size_t total, size_t now)> onProgress);

    /// 安装更新：替换运行中的 NRO 文件（仅 Switch 平台）
    bool install();

private:
    AppUpdater() = default;

    UpdateInfo m_info;
    std::vector<uint8_t> m_downloadedData; // 下载的 NRO 数据
};

} // namespace beiklive
