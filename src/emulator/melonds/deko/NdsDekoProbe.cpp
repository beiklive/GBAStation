#include "emulator/melonds/deko/NdsDekoProbe.hpp"

#include <borealis/core/logger.hpp>

#ifdef __SWITCH__
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include <deko3d.hpp>
#include <switch.h>
#endif

namespace beiklive {

namespace {

#ifdef __SWITCH__
constexpr int kFramebufferCount = 2;
constexpr uint32_t kCommandMemorySize = 0x4000;
constexpr int kMaxProbeLevel = 10;
constexpr const char* kCheckpointPaths[] = {
    "/GBAStation/log/NdsDekoProbe.checkpoint.log",
    "sdmc:/GBAStation/log/NdsDekoProbe.checkpoint.log",
};

uint32_t alignUp(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void dekoDebugOutput(void*, const char*, DkResult result, const char* message)
{
    brls::Logger::warning("NdsDekoProbe: deko debug result={} message={}", static_cast<int>(result), message ? message : "");
}

void checkpoint(const char* format, ...)
{
    char buffer[512] = {};

    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    brls::Logger::info("NdsDekoProbe: {}", buffer);

    for (const char* path : kCheckpointPaths)
    {
        FILE* fp = std::fopen(path, "a");
        if (!fp)
            continue;

        std::fprintf(fp, "%s\n", buffer);
        std::fflush(fp);
        std::fclose(fp);
        break;
    }
}

void resetCheckpointLog()
{
    for (const char* path : kCheckpointPaths)
    {
        FILE* fp = std::fopen(path, "w");
        if (!fp)
            continue;

        std::fprintf(fp, "NdsDekoProbe checkpoint reset\n");
        std::fflush(fp);
        std::fclose(fp);
        break;
    }
}

bool getDefaultResolution(uint32_t& width, uint32_t& height)
{
    int32_t detectedWidth = 0;
    int32_t detectedHeight = 0;
    if (R_SUCCEEDED(appletGetDefaultDisplayResolution(&detectedWidth, &detectedHeight)) &&
        detectedWidth > 0 && detectedHeight > 0)
    {
        width = static_cast<uint32_t>(detectedWidth);
        height = static_cast<uint32_t>(detectedHeight);
        return true;
    }

    if (appletGetOperationMode() == AppletOperationMode_Console)
    {
        width = 1920;
        height = 1080;
    }
    else
    {
        width = 1280;
        height = 720;
    }
    return false;
}
#endif

} // namespace

NdsDekoProbeResult RunNdsDekoProbe(const NdsDekoProbeOptions& options)
{
    NdsDekoProbeResult result;
    result.requestedLevel = options.level;

#ifndef __SWITCH__
    result.message = "Deko3D probe is only available on Switch";
    return result;
#else
    result.supported = true;
    const int requestedLevel = options.level < 1 ? 1 : (options.level > kMaxProbeLevel ? kMaxProbeLevel : options.level);
    const int frameCount = options.frameCount > 0 ? options.frameCount : 60;
    result.requestedLevel = requestedLevel;

    if (requestedLevel == 1)
        resetCheckpointLog();

    uint32_t framebufferWidth = 1280;
    uint32_t framebufferHeight = 720;
    const bool detectedResolution = getDefaultResolution(framebufferWidth, framebufferHeight);
    checkpoint("probe begin, level=%d, %ux%u, detected=%s",
               requestedLevel,
               framebufferWidth,
               framebufferHeight,
               detectedResolution ? "yes" : "fallback");

    dk::Device device;
    dk::Queue queue;
    dk::CmdBuf cmdBuf;
    dk::Swapchain swapchain;
    dk::MemBlock imageMemory;
    dk::MemBlock commandMemory;
    std::array<dk::Image, kFramebufferCount> framebuffers;
    std::array<DkCmdList, kFramebufferCount> clearCommands{};

    bool deviceCreated = false;
    bool queueCreated = false;
    bool cmdBufCreated = false;
    bool imageMemoryCreated = false;
    bool commandMemoryCreated = false;
    bool swapchainCreated = false;

    auto cleanup = [&]() {
        if (queueCreated)
            queue.waitIdle();
        if (swapchainCreated)
            swapchain.destroy();
        if (cmdBufCreated)
            cmdBuf.destroy();
        if (commandMemoryCreated)
            commandMemory.destroy();
        if (imageMemoryCreated)
            imageMemory.destroy();
        if (queueCreated)
            queue.destroy();
        if (deviceCreated)
            device.destroy();
    };

    auto finish = [&](int reachedLevel, bool success, const std::string& message) {
        result.reachedLevel = reachedLevel;
        result.success = success;
        result.message = message;
        checkpoint("finish level=%d success=%s message=%s", reachedLevel, success ? "true" : "false", message.c_str());
        cleanup();
        checkpoint("returned to borealis");
        return result;
    };

    checkpoint("level 1 create device begin");
    device = dk::DeviceMaker{}.setCbDebug(dekoDebugOutput).create();
    deviceCreated = true;
    checkpoint("level 1 create device end");
    if (requestedLevel <= 1)
        return finish(1, true, "Deko device create/destroy completed");

    checkpoint("level 2 create queue begin");
    queue = dk::QueueMaker{device}
                .setFlags(DkQueueFlags_Graphics)
                .setCommandMemorySize(DK_QUEUE_MIN_CMDMEM_SIZE * 2)
                .setFlushThreshold(DK_QUEUE_MIN_CMDMEM_SIZE)
                .create();
    queueCreated = true;
    checkpoint("level 2 create queue end");
    if (requestedLevel <= 2)
        return finish(2, true, "Deko device+queue create/destroy completed");

    if (requestedLevel == 6)
    {
        checkpoint("level 6 get default nwindow begin");
        NWindow* defaultWindow = nwindowGetDefault();
        checkpoint("level 6 get default nwindow end, ptr=%p", static_cast<void*>(defaultWindow));
        if (!defaultWindow)
            return finish(6, false, "Deko default nwindow is null");
        return finish(6, true, "Deko default nwindow lookup completed");
    }

    const bool needsPresentLayout = requestedLevel >= 4;
    checkpoint("level %d create %s framebuffer layout begin",
               needsPresentLayout ? 4 : 3,
               needsPresentLayout ? "present-capable" : "offscreen");
    dk::ImageLayout framebufferLayout;
    dk::ImageLayoutMaker{device}
        .setFlags(needsPresentLayout
                      ? (DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_Usage2DEngine)
                      : (DkImageFlags_UsageRender | DkImageFlags_Usage2DEngine))
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(framebufferWidth, framebufferHeight)
        .initialize(framebufferLayout);
    checkpoint("level %d framebuffer layout end, presentFlag=%s, size=%u, align=%u",
               needsPresentLayout ? 4 : 3,
               needsPresentLayout ? "yes" : "no",
               framebufferLayout.getSize(),
               framebufferLayout.getAlignment());
    if (requestedLevel <= 4 && needsPresentLayout)
        return finish(4, true, "Deko present-capable framebuffer layout completed");

    const uint32_t framebufferStride = alignUp(framebufferLayout.getSize(), framebufferLayout.getAlignment());
    const uint32_t imageMemorySize = alignUp(framebufferStride * kFramebufferCount, DK_MEMBLOCK_ALIGNMENT);
    checkpoint("create image memory begin, stride=%u, total=%u",
               framebufferStride,
               imageMemorySize);
    imageMemory = dk::MemBlockMaker{device, imageMemorySize}
                      .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
                      .create();
    imageMemoryCreated = true;
    checkpoint("create image memory end");

    std::array<DkImage const*, kFramebufferCount> framebufferRefs{};
    for (int i = 0; i < kFramebufferCount; ++i)
    {
        checkpoint("framebuffer image %d initialize begin", i);
        framebuffers[i].initialize(framebufferLayout, imageMemory, framebufferStride * i);
        framebufferRefs[i] = &framebuffers[i];
        checkpoint("framebuffer image %d initialize end", i);
    }
    checkpoint("framebuffer images initialized");
    if (requestedLevel <= 3)
        return finish(3, true, "Deko framebuffer create/destroy completed");
    if (requestedLevel <= 5)
        return finish(5, true, "Deko present-capable framebuffer create/destroy completed");

    checkpoint("level 7 get default nwindow before swapchain begin");
    NWindow* defaultWindow = nwindowGetDefault();
    checkpoint("level 7 get default nwindow before swapchain end, ptr=%p", static_cast<void*>(defaultWindow));
    if (!defaultWindow)
        return finish(7, false, "Deko default nwindow is null before swapchain");

    checkpoint("level 7 construct swapchain maker begin");
    dk::SwapchainMaker swapchainMaker{device, defaultWindow, framebufferRefs};
    checkpoint("level 7 construct swapchain maker end");

    checkpoint("level 7 swapchain create begin");
    swapchain = swapchainMaker.create();
    swapchainCreated = true;
    checkpoint("level 7 swapchain create end");
    if (requestedLevel <= 7)
        return finish(7, true, "Deko swapchain create/destroy completed");

    checkpoint("level 8 set swap interval begin");
    swapchain.setSwapInterval(1);
    checkpoint("level 8 set swap interval end");
    if (requestedLevel <= 8)
        return finish(8, true, "Deko swapchain setSwapInterval completed");

    checkpoint("level 9 create command lists begin");
    cmdBuf = dk::CmdBufMaker{device}.create();
    cmdBufCreated = true;
    checkpoint("level 9 cmdBuf created");
    commandMemory = dk::MemBlockMaker{device, kCommandMemorySize}
                        .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
                        .create();
    commandMemoryCreated = true;
    checkpoint("level 9 command memory created");
    cmdBuf.addMemory(commandMemory, 0, kCommandMemorySize);
    checkpoint("level 9 command memory attached");

    for (int i = 0; i < kFramebufferCount; ++i)
    {
        checkpoint("level 9 clear command list %d begin", i);
        dk::ImageView colorTarget{framebuffers[i]};
        const float red = i == 0 ? 0.05f : 0.10f;
        const float green = i == 0 ? 0.15f : 0.05f;
        const float blue = i == 0 ? 0.35f : 0.20f;

        cmdBuf.bindRenderTargets(&colorTarget);
        cmdBuf.setViewports(0, {{0.0f, 0.0f, static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight), 0.0f, 1.0f}});
        cmdBuf.setScissors(0, {{0, 0, framebufferWidth, framebufferHeight}});
        cmdBuf.clearColor(0, DkColorMask_RGBA, red, green, blue, 1.0f);
        clearCommands[i] = cmdBuf.finishList();
        checkpoint("level 9 clear command list %d end", i);
    }

    checkpoint("level 9 create command lists end");
    if (requestedLevel <= 9)
        return finish(9, true, "Deko command lists create/destroy completed");

    checkpoint("level 10 acquire/clear/present begin");

    for (int frame = 0; frame < frameCount; ++frame)
    {
        checkpoint("level 10 acquire frame %d begin", frame);
        const int slot = queue.acquireImage(swapchain);
        checkpoint("level 10 acquire frame %d end, slot=%d", frame, slot);
        checkpoint("level 10 submit frame %d begin", frame);
        queue.submitCommands(clearCommands[slot]);
        checkpoint("level 10 submit frame %d end", frame);
        checkpoint("level 10 present frame %d begin", frame);
        queue.presentImage(swapchain, slot);
        checkpoint("level 10 present frame %d end", frame);
        result.presentedFrames += 1;

        if ((frame % 30) == 0)
            checkpoint("present frame %d", frame);
    }

    queue.waitIdle();
    checkpoint("level 10 queue idle, frames=%d", result.presentedFrames);

    return finish(10, true, "Deko present completed");
#endif
}

} // namespace beiklive
