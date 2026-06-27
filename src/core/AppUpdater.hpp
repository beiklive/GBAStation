#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <vector>

namespace beiklive {

struct UpdateInfo {
    std::string version;      // 远程版本号
    std::string changelog;    // 更新提示
    std::string downloadUrl;  // 下载链接
    size_t      fileSize = 0; // 文件大小（字节）
    bool hasUpdate = false;   // 是否有更新
};

class AppUpdater {
public:
    static AppUpdater& instance();

    /// 异步检查更新，不阻塞
    void check();

    /// 同步检查远程版本，基于 APP_VERSION 判断是否有更新
    bool checkSync();

    /// 获取最新版本信息
    const UpdateInfo& info() const { return m_info; }

    /// 是否有可用更新
    bool hasUpdate() const { return m_info.hasUpdate; }

    /// 下载 NRO 到临时目录（Switch 平台），返回是否成功
    /// onProgress: (totalBytes, downloadedBytes) → 返回 false 可中断
    bool download(std::function<bool(size_t total, size_t now)> onProgress);

    /// 准备安装：校验缓存文件是否就绪
    bool install();

    /// 完成安装：romfsExit + 替换 NRO 文件（必须在 UI 线程调用，紧接 quit）
    bool finishInstall();

    /// 取消正在进行的网络操作
    void abort() { m_aborted.store(true); }

private:
    AppUpdater() = default;

    UpdateInfo m_info;
    std::vector<uint8_t> m_downloadedData;
    std::atomic<bool> m_aborted{false};
};

} // namespace beiklive
