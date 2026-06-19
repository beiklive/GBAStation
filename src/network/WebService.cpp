#include "WebService.h"

#include "HttpServer.h"
#include "NetworkManager.h"

#include <borealis.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive::network::WebService
{
namespace
{
NetworkManager g_network;
HttpServer g_server;
bool g_networkReady = false;
bool g_keepAwakeRequested = false;
bool g_keepAwakeActive = false;
std::string g_lastError;

void setKeepAwake(bool enabled)
{
#ifdef __SWITCH__
    Result rc = appletSetMediaPlaybackState(enabled);
    if (R_SUCCEEDED(rc))
    {
        g_keepAwakeActive = enabled;
        brls::Logger::info("Web service keep-awake {}", enabled ? "enabled" : "disabled");
    }
    else
    {
        g_keepAwakeActive = false;
        brls::Logger::warning("Web service keep-awake {} failed: 0x{:X}", enabled ? "enable" : "disable", rc);
    }
#else
    g_keepAwakeActive = false;
    (void)enabled;
#endif
}

void enableKeepAwake()
{
    if (g_keepAwakeRequested)
        return;
    g_keepAwakeRequested = true;
    setKeepAwake(true);
}

void disableKeepAwake()
{
    if (!g_keepAwakeRequested)
        return;
    g_keepAwakeRequested = false;
    setKeepAwake(false);
}
}

bool Start(int port)
{
    g_lastError.clear();
    if (g_server.IsRunning())
        return true;

    if (!g_networkReady)
        g_networkReady = g_network.Initialize();

    if (!g_networkReady)
    {
        g_lastError = "网络初始化失败";
        return false;
    }

    for (int candidate = port; candidate < port + 10; ++candidate)
    {
        if (g_server.Start(candidate))
        {
            enableKeepAwake();
            return true;
        }
    }

    g_lastError = "HTTP 端口监听失败";
    return false;
}

void Stop()
{
    g_server.Stop();
    disableKeepAwake();
    if (g_networkReady)
    {
        g_network.Shutdown();
        g_networkReady = false;
    }
}

void Update()
{
    bool wasRunning = g_server.IsRunning();
    g_server.Update();
    if (wasRunning && !g_server.IsRunning() && g_networkReady)
    {
        disableKeepAwake();
        g_network.Shutdown();
        g_networkReady = false;
    }
}

bool IsRunning()
{
    return g_server.IsRunning();
}

int Port()
{
    return g_server.Port();
}

std::string Url()
{
    std::string ip = brls::Application::getPlatform()->getIpAddress();
    if (ip.empty())
        ip = "127.0.0.1";

    int port = Port();
    if (port <= 0)
        port = 8080;

    return "http://" + ip + ":" + std::to_string(port);
}

std::string LastError()
{
    return g_lastError.empty() ? "未知错误" : g_lastError;
}

std::string KeepAwakeMessage()
{
#ifdef __SWITCH__
    return g_keepAwakeActive ? "已在 Web 服务运行期间保持屏幕常亮。"
                             : "无法自动保持屏幕常亮，请在使用期间避免让 Switch 熄屏。";
#else
    return "桌面调试环境无需保持 Switch 屏幕常亮。";
#endif
}

} // namespace beiklive::network::WebService
