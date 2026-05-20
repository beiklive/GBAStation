#include "platform/PlatformCommon.hpp"

#include <cstdio>
#include <borealis.hpp>

namespace beiklive::melonds::platform
{

static struct PlatformContext {
    std::string ndsSavePath;
    std::string gbaSavePath;
    std::string firmwarePath;
    std::function<void()> stopCallback;
} g_ctx;

void setNDSSavePath(const std::string& path) { g_ctx.ndsSavePath = path; }
void setGBASavePath(const std::string& path) { g_ctx.gbaSavePath = path; }
void setFirmwarePath(const std::string& path) { g_ctx.firmwarePath = path; }
void setStopCallback(std::function<void()> cb) { g_ctx.stopCallback = std::move(cb); }

void writeNDSSave(const uint8_t* data, uint32_t len, uint32_t offset, uint32_t writelen)
{
    (void)offset; (void)writelen;
    if (g_ctx.ndsSavePath.empty() || !data || len == 0) return;
    FILE* f = fopen(g_ctx.ndsSavePath.c_str(), "wb");
    if (!f) return;
    fwrite(data, len, 1, f);
    fclose(f);
}

void writeGBASave(const uint8_t* data, uint32_t len, uint32_t offset, uint32_t writelen)
{
    (void)offset; (void)writelen;
    if (g_ctx.gbaSavePath.empty() || !data || len == 0) return;
    FILE* f = fopen(g_ctx.gbaSavePath.c_str(), "wb");
    if (!f) return;
    fwrite(data, len, 1, f);
    fclose(f);
}

void writeDateTime(int year, int month, int day, int hour, int minute, int second)
{
    (void)year; (void)month; (void)day; (void)hour; (void)minute; (void)second;
}

void signalStop(int reason)
{
    if (g_ctx.stopCallback)
        g_ctx.stopCallback();
}

} // namespace beiklive::melonds::platform
